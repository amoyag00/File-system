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
static int create_aux(struct inode *inode, struct dentry *dentry, umode_t mode);
int assoofs_get_freeblock(struct super_block *sb, uint64_t * free_block_number);
void assoofs_inode_add(struct super_block *sb, struct assoofs_inode_info *ino_info);
void assoofs_sb_sync(struct super_block *sb);
int assoofs_inode_save(struct super_block *sb, struct assoofs_inode_info *ino_info);
struct assoofs_inode_info *assoofs_inode_search(struct super_block *sb,struct assoofs_inode_info *start,
	struct assoofs_inode_info *target);

static DEFINE_MUTEX(assoofs_mutex);
static DEFINE_MUTEX(mutex_write);



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
	brelse(buff);

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
	brelse(buffer);
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
	brelse(buffer);
	return NULL;
}

static int assoofs_create(struct inode *dir , struct dentry *dentry , umode_t mode , bool excl){
	printk("Creando inodo\n");
	return create_aux(dir, dentry,mode);
}

static int assoofs_mkdir(struct inode *dir , struct dentry *dentry , umode_t mode){
	printk("Creando directorio\n");
	return create_aux(dir, dentry, S_IFDIR | mode);
	
	
}

static int create_aux(struct inode *dir, struct dentry *dentry, umode_t mode){
	struct inode *inode;
	struct super_block *sb;
	struct assoofs_inode_info *parent_dir_info, *ino_info;
	struct buffer_head *buffer_head;
	struct assoofs_super_block_info *sb_info;	
	struct assoofs_dir_record_entry *dir_contents_datablock;
	uint64_t count;

	//Se crea un inodo para el nuevo directorio/archivo
	sb = dir->i_sb;
	sb_info=sb->s_fs_info;
	mutex_lock_interruptible(&mutex_write);
	count=sb_info->inodes_count;
	mutex_unlock(&mutex_write);
	if(count==ASSOOFS_MAX_FILESYSTEM_OBJECTS_SUPPORTED){
		printk("No se pueden crear más files\n");
		return 0;
	}
	inode = new_inode(sb);
	
	inode->i_sb=sb;
	inode->i_op=&assoofs_inode_ops;
	inode->i_atime=CURRENT_TIME;
	inode->i_ctime=CURRENT_TIME;
	inode->i_mtime=CURRENT_TIME;
	inode->i_ino=(count+ ASSOOFS_START_INO - ASSOOFS_RESERVED_INODES+1);
	ino_info=kmalloc(sizeof(struct assoofs_inode_info),GFP_KERNEL);
	ino_info->inode_no = inode->i_ino;
	inode->i_private=ino_info;
	ino_info->mode=mode;
	
	
	if(S_ISDIR(mode)){
		ino_info->dir_children_count=0;//Al crear un directorio no tendrá hijos
		inode->i_fop=&assoofs_dir_operations;
		printk("Es un directorio\n");
	}else if(S_ISREG(mode)){
		ino_info->file_size=0;//Al crear un fichero su tamaño inicial será 0.
		inode->i_fop=&assoofs_file_operations;
		printk("Es un fichero\n");
	}
	
	if(assoofs_get_freeblock(sb,&ino_info->data_block_number)<0){
		return -1;
	}

	assoofs_inode_add(sb,ino_info);

	parent_dir_info=dir->i_private;

	buffer_head = sb_bread(sb, parent_dir_info->data_block_number);
	dir_contents_datablock=(struct assoofs_dir_record_entry *)buffer_head->b_data;
	dir_contents_datablock += parent_dir_info->dir_children_count;
	dir_contents_datablock->inode_no = ino_info->inode_no;
	strcpy(dir_contents_datablock->filename, dentry->d_name.name);

	mark_buffer_dirty(buffer_head);
	sync_dirty_buffer(buffer_head);
	brelse(buffer_head);
	mutex_lock_interruptible(&mutex_write);
	parent_dir_info->dir_children_count++;
	

	assoofs_inode_save(sb,parent_dir_info);
	mutex_unlock(&mutex_write);
	inode_init_owner(inode, dir, mode);
	d_add(dentry, inode);
	return 0;
}
int assoofs_get_freeblock(struct super_block *sb, uint64_t * free_block_number){
	//Hay que encontrar un bloque vacío
	/* 1.- Suponer un mapa de bits tal que 100011001010
	*  2.- Crear una máscara de bits tal que 1+i0's; con i=5 sería 100000
	*  3.- Hacer un and lógico del mapa y la máscara: 100011001010 & [000000]100000 ; 
	*  4.- Si el bloque en la posición i estaba libre (era un 1), al hacer el and lógico eso nos dará un 1.
	*  5.- Se sale del bucle. Nos encontramos con que el primer bloque libre está en la posición i;
	*/
	struct assoofs_super_block_info *sb_info;
	int i;
	sb_info=sb->s_fs_info;
	mutex_lock_interruptible(&assoofs_mutex);
	for (i = ASSOOFS_RESERVED_INODES; i <ASSOOFS_MAX_FILESYSTEM_OBJECTS_SUPPORTED; i++) {
		if (sb_info->free_blocks & (1 << i)) {
			break;
		}
	}

	if( i == ASSOOFS_MAX_FILESYSTEM_OBJECTS_SUPPORTED){
		printk("No quedan huecos libres\n");
		return -1;
	}

	*free_block_number=i;
	printk("Encontrado bloque libre, numero: %d\n",i);
	/* Hay que decir al mapa de bits que el bloque que estaba libre ya no lo está
	* 1.- Usamos la máscara de bits de la última iteración y la negamos . Ej: 1000->0111
	* 2.- Al hacer un and lógico del mapa y esta máscara negada, la posición en la que había un 1 ahora
	* tendrá un 0 y el resto quedará tal y como estaba ya que 1 & X= X;  0 & X = 0.
	*/
	sb_info->free_blocks &= ~(1 << i);
	assoofs_sb_sync(sb);
	mutex_unlock(&assoofs_mutex);
	return 0;
}

void assoofs_inode_add(struct super_block *sb, struct assoofs_inode_info *ino_info){
	struct assoofs_super_block_info *sb_info;
	struct buffer_head *buffer_head;
	struct assoofs_inode_info *iterator;
	sb_info=sb->s_fs_info;

	//Se guardan los cambios en al almacen de inodos
	mutex_lock_interruptible(&mutex_write);
	buffer_head = sb_bread(sb, ASSOOFS_INODESTORE_BLOCK_NUMBER);
	iterator=(struct assoofs_inode_info *)buffer_head->b_data;
	mutex_lock_interruptible(&assoofs_mutex);
	iterator+=sb_info->inodes_count;
	memcpy(iterator, ino_info, sizeof(struct assoofs_inode_info));
	sb_info->inodes_count++;
	printk("Inode added to the Inode Store \n");
	mark_buffer_dirty(buffer_head);
	assoofs_sb_sync(sb);
	brelse(buffer_head);
	mutex_unlock(&assoofs_mutex);
	mutex_unlock(&mutex_write);
}

void assoofs_sb_sync(struct super_block *sb){
	struct buffer_head *buffer_head;
	struct assoofs_super_block_info *sb_info;
	sb_info = sb->s_fs_info;

	//Se guardan estos cambios en el superbloque
	buffer_head = sb_bread(sb, ASSOOFS_SUPERBLOCK_BLOCK_NUMBER);
	buffer_head->b_data = (char *)sb_info;
	mark_buffer_dirty(buffer_head);
	sync_dirty_buffer(buffer_head);
	brelse(buffer_head);
	printk("Superbloque sincronizado\n");

	
}
int assoofs_inode_save(struct super_block *sb, struct assoofs_inode_info *parent_dir_info){
	struct assoofs_inode_info *iterator;
	struct buffer_head *buffer_head;
	printk("Guardando los datos del inodo modificado\n");
	buffer_head = sb_bread(sb, ASSOOFS_INODESTORE_BLOCK_NUMBER);
	mutex_lock_interruptible(&assoofs_mutex);
	iterator= assoofs_inode_search(sb,(struct assoofs_inode_info *)buffer_head->b_data, parent_dir_info);//iterator= parent_dir_info;
	memcpy(iterator, parent_dir_info, sizeof(*iterator));
	mark_buffer_dirty(buffer_head);
	sync_dirty_buffer(buffer_head);
	brelse(buffer_head);
	mutex_unlock(&assoofs_mutex);
	return 0;
}

struct assoofs_inode_info *assoofs_inode_search(struct super_block *sb,struct assoofs_inode_info *start,
	struct assoofs_inode_info *target){
	struct assoofs_super_block_info *sb_info;
	uint64_t count = 0;
	printk("Buscando inodo\n");
	//Busca y devuelve la posición de memoria donde se encuentra un inodo
	//En concreto lo uso para extraer la dir de memoria en la que se encuentra el dir padre dentro del inode_store.
	sb_info=sb->s_fs_info;
	while (start->inode_no != target->inode_no && count < sb_info->inodes_count) {
		count++;
		start++;
	}

	if (start->inode_no == target->inode_no) {
		return start;
	}

	return NULL;
}

ssize_t  assoofs_read(struct  file *filp , char  __user *buf , size_t len , loff_t *ppos){
	struct inode *inode; 
	struct super_block *sb;
	struct assoofs_inode_info *ino_info;
	struct buffer_head *buffer;
	char *buff_char;
	int n_bytes;
	inode = filp->f_path.dentry->d_inode;

	sb=inode->i_sb;
	ino_info=inode->i_private;
	if(*ppos>=ino_info->file_size){
		return 0;
	}
	buffer=sb_bread(sb,ino_info->data_block_number);
	buff_char=(char *)buffer->b_data;
	n_bytes=min(len, (size_t)ino_info->file_size);
	copy_to_user(buf,buff_char,n_bytes);
	*ppos+=n_bytes;
	printk("Se han leído %d bytes\n",n_bytes);
	brelse(buffer);
	return n_bytes;

}

ssize_t  assoofs_write(struct  file *filp , const  char  __user *buf , size_t len , loff_t *ppos){
	struct inode *inode; 
	struct super_block *sb;
	struct assoofs_inode_info *ino_info;
	struct buffer_head *buffer;
	char * buff_char;
	
	inode = filp->f_path.dentry->d_inode;
	sb=inode->i_sb;
	ino_info=inode->i_private;
	buffer=sb_bread(sb,ino_info->data_block_number);
	if (!buffer) {
		printk("Fallo al leer el bloque\n");
		return 0;
	}
	buff_char=(char *)buffer->b_data;
	buff_char+=*ppos;
	copy_from_user(buff_char,buf,len);
	*ppos+=len;
	mark_buffer_dirty(buffer);
	printk("Marked as dirty\n");
	sync_dirty_buffer(buffer);
	printk("Synchronized\n");
	brelse(buffer);
	mutex_lock_interruptible(&mutex_write);
	ino_info->file_size=*ppos;
	assoofs_inode_save(sb, ino_info);
	mutex_unlock(&mutex_write);
	printk("Se han escrito %lu bytes\n",len);
	
	return len;
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



