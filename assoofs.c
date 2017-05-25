#include  <linux/module.h>         /*  Needed  by all  modules  */
#include  <linux/kernel.h>         /*  Needed  for  KERN_INFO   */
#include  <linux/init.h>           /*  Needed  for  the  macros  */
#include  <linux/fs.h>             /*  libfs  stuff             */
#include  <asm/uaccess.h>          /*  copy_to_user            */
#include  <linux/buffer_head.h>    /*  buffer_head              */
#include  <linux/slab.h>
#include "assoofs.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alejandro Moya García");


int assoofs_fill_super(struct super_block *sb, void *data , int silent);
static  struct  dentry *assoofs_mount(struct  file_system_type *fs_type ,int flags , const  char *dev_name , void *data);
struct dentry *assoofs_lookup(struct inode *parent_inode , struct dentry *child_dentry , unsigned int flags);
static int assoofs_create(struct inode *dir , struct dentry *dentry , umode_t mode , bool excl);
static int assoofs_mkdir(struct inode *dir , struct dentry *dentry , umode_t mode);
ssize_t  assoofs_read(struct  file *filp , char  __user *buf , size_t len , loff_t *ppos);
ssize_t  assoofs_write(struct  file *filp , const  char  __user *buf , size_t len , loff_t *ppos);
static  int  assoofs_iterate(struct  file *filp , struct  dir_context *ctx);
struct assoofs_inode_info *assoofs_get_inode_info(struct super_block *sb, uint64_t ino);
struct inode *assoofs_get_inode(struct super_block *sb, uint64_t ino);


static  struct  file_system_type  assoofs_type = {
		.owner    = THIS_MODULE ,
		.name     = "assoofs",
		.mount    = assoofs_mount ,
		.kill_sb = kill_litter_super ,
	};

static  struct  inode_operations  assoofs_inode_ops = {
	.create = assoofs_create ,
	.lookup = assoofs_lookup ,
	.mkdir = assoofs_mkdir ,
};

const  struct  file_operations  assoofs_file_operations = {
	.read = assoofs_read ,
	.write = assoofs_write ,
};

const  struct  file_operations  assoofs_dir_operations = {
	.owner = THIS_MODULE ,
	.iterate = assoofs_iterate ,
};

static  const  struct  super_operations  assoofs_sops = {//Hay que añadir más operaciones
	.drop_inode     = generic_delete_inode ,
};


static  int  __init  assoofs_init(void)
{
	 if(register_filesystem(&assoofs_type)!=0){
	 	printk("No se ha registrado correctamente\n");
	 	return -1;
	 }else{
	 	printk("Registrado correctamente\n");
	 }
	return  0;
}


static  void  __exit  assoofs_exit(void)
{
	 if(unregister_filesystem(&assoofs_type)!=0){
	 	printk("No se ha unregistrado correctamente\n");
	 	
	 }else{
	 	printk("Unregistrado correctamente\n");
	 } 
}

static  struct  dentry *assoofs_mount(struct  file_system_type *fs_type ,
int flags , const  char *dev_name , void *data)
{
	struct dentry *value=mount_bdev(fs_type ,flags ,dev_name ,data ,assoofs_fill_super);
	printk("Montado correctamente\n");
	return value;
}

int assoofs_fill_super(struct super_block *sb, void *data , int silent)
{
	struct buffer_head *buff;
	struct assoofs_super_block_info *asb;
	struct inode *root_inode;

	//Leer información del superbloque
	buff=sb_bread(sb,ASSOOFS_SUPERBLOCK_BLOCK_NUMBER);//El super bloque está en el bloque 0
	asb=(struct assoofs_super_block_info *)buff->b_data;//No es el superbloque como tal, es información del superbloque.

	//Comprobar parámetros
	if(asb->magic!=ASSOOFS_MAGIC){
		printk("El número mágico no coincide con el esperado\n");
		return -1;
	}

	if(asb->block_size!=ASSOOFS_DEFAULT_BLOCK_SIZE){
		printk("El tamaño de bloque no coincide con el esperado\n");
		return -1;
	}

	//Escribir información.
	sb->s_magic=ASSOOFS_MAGIC;
	sb->s_maxbytes=ASSOOFS_DEFAULT_BLOCK_SIZE;
	sb->s_fs_info=asb;//Guardamos el superbloque_info porque probablemente lo necesitaremos luego.
	sb->s_op=&assoofs_sops;//direccion de variable tipo super_operation que será global. Operaciones que soporta el superbloque
	printk("Superbloque- número mágico %lu\n",sb->s_magic);

	//Crear el inodo raíz
	root_inode=new_inode(sb);
	inode_init_owner(root_inode,NULL,S_IFDIR);
	root_inode->i_ino=ASSOOFS_ROOTDIR_INODE_NUMBER;
	root_inode->i_sb=sb;
	root_inode->i_atime=CURRENT_TIME;
	root_inode->i_mtime=CURRENT_TIME;
	root_inode->i_ctime=CURRENT_TIME;
	root_inode->i_op=&assoofs_inode_ops;
	root_inode->i_fop=&assoofs_dir_operations;
	root_inode->i_private= assoofs_get_inode_info(sb,ASSOOFS_ROOTDIR_INODE_NUMBER);//el inode_info, similar al asb en  s_fs_info
	
	sb->s_root=d_make_root(root_inode);
	printk("inodo raíz creado\n");


	return 0;

}

struct assoofs_inode_info *assoofs_get_inode_info(struct super_block *sb,uint64_t ino){
	int i=0;
	struct buffer_head *buffer;
	struct assoofs_inode_info *ino_info;
	struct assoofs_super_block_info *sb_info;
	buffer=sb_bread(sb,ASSOOFS_INODESTORE_BLOCK_NUMBER);
	sb_info=sb->s_fs_info;
	ino_info=(struct assoofs_inode_info *)buffer->b_data;
	for (i=0;i<sb_info->inodes_count;i++){
		if(ino==ino_info->inode_no){
			brelse(buffer);
			return ino_info;
		}
		ino_info++;
	}
	return NULL;
}

struct inode *assoofs_get_inode(struct super_block *sb, uint64_t ino){
	struct assoofs_inode_info *ino_info;
	struct inode *inode;
	ino_info=assoofs_get_inode_info(sb,ino);
	inode= new_inode(sb);
	inode->i_sb=sb;
	inode->i_op=&assoofs_inode_ops;
	inode->i_atime=CURRENT_TIME;
	inode->i_ctime=CURRENT_TIME;
	inode->i_mtime=CURRENT_TIME;
	inode->i_private=ino_info;
	inode->i_ino=ino;
	if(S_ISDIR(ino_info->mode)){
		inode->i_fop=&assoofs_dir_operations;
	}else if (S_ISREG(ino_info->mode)){
		inode->i_fop=&assoofs_file_operations;
	}
	//Después de llamar a esta función hay que llamar a init_owner diciéndole el inodo padre.
	//Aquí no podemos porque no sabemos cuál es.
	return inode;

}

struct dentry *assoofs_lookup(struct inode *parent_inode , struct dentry *child_dentry , unsigned int flags){
	struct super_block *sb;
	struct buffer_head *buffer;
	struct assoofs_dir_record_entry *record;
	struct inode *inode;
	struct assoofs_inode_info *ino_info,*ino_info2;
	struct qstr dname;
	int i=0;
	sb=parent_inode->i_sb;
	ino_info=parent_inode->i_private;
	buffer=sb_bread(sb,ino_info->data_block_number);
	record=(struct assoofs_dir_record_entry *)buffer->b_data;
	
	for(i=0;i<ino_info->dir_children_count;i++){
		dname=child_dentry->d_name;
		if(strcmp(dname.name,record->filename)==0){
			inode=assoofs_get_inode(sb,record->inode_no);
			ino_info2=inode->i_private;
			inode_init_owner(inode,parent_inode,ino_info2->mode);
			d_add(child_dentry,inode);//Esto lo mantiene en memoria
			brelse(buffer);
			printk("inodo encontrado\n");
			return NULL;
		}
		record++;

	}
	
	return NULL;
}

static int assoofs_create(struct inode *dir , struct dentry *dentry , umode_t mode , bool excl){
	printk("inodo creado\n");
	return 0;
}

static int assoofs_mkdir(struct inode *dir , struct dentry *dentry , umode_t mode){
	printk("Directorio creado\n");
	return 0;
}

ssize_t  assoofs_read(struct  file *filp , char  __user *buf , size_t len , loff_t *ppos){
	//Leer el bloque
	//DE filp se saca el numero de inodo
	//guardar lo leido (buffer_head) en buf
	//usar copy_to_user
	printk("Se han leído %lu bytes\n",len);
	return 0;
}

ssize_t  assoofs_write(struct  file *filp , const  char  __user *buf , size_t len , loff_t *ppos){
	printk("Se han escrito %lu bytes\n",len);
	return 0;
}

static  int  assoofs_iterate(struct  file *filp , struct  dir_context *ctx){
	struct inode *inode;
	struct buffer_head *buffer;
	struct assoofs_dir_record_entry *record;
	struct super_block *sb;
	struct assoofs_inode_info *ino_info;
	int i=0;
	inode=filp->f_path.dentry->d_inode;
	sb=inode->i_sb;
	ino_info=inode->i_private;
	if(ctx->pos){
		return 0;
	}
	buffer=sb_bread(sb,ino_info->data_block_number);
	record=(struct assoofs_dir_record_entry*)buffer->b_data;
	printk("Dir children count %llu \n ",ino_info->dir_children_count);
	for (i=0;i<ino_info->dir_children_count;i++){
		printk("Iteración: %d",i);
		dir_emit(ctx,record->filename,ASSOOFS_FILENAME_MAXLEN,record->inode_no,DT_UNKNOWN);//Actualiza el contexto
		ctx->pos+=sizeof(struct assoofs_dir_record_entry);
		record++;
	}
	brelse(buffer);
	printk("Iterated\n");
	return 0;

}

module_init(assoofs_init);
module_exit(assoofs_exit);



