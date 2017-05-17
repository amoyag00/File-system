#include  <linux/module.h>         /*  Needed  by all  modules  */
#include  <linux/kernel.h>         /*  Needed  for  KERN_INFO   */
#include  <linux/init.h>           /*  Needed  for  the  macros  */
#include  <linux/fs.h>             /*  libfs  stuff             */
#include  <asm/uaccess.h>          /*  copy_to_user            */
#include  <linux/buffer_head.h>    /*  buffer_head              */
#include  <linux/slab.h>
#include "assoofs.h"  

int assoofs_fill_super(struct super_block *sb, void *data , int silent);

MODULE_LICENSE("GPL");

MODULE_AUTHOR("Angel  Manuel  Guerrero  Higueras");


static  struct  dentry *assoofs_mount(struct  file_system_type *fs_type ,
int flags , const  char *dev_name , void *data)
{
	struct dentry *value=mount_bdev(fs_type ,flags ,dev_name ,data ,assoofs_fill_super);
	printk("Montado correctamente\n");
	return value;
}


static  struct  file_system_type  assoofs_type = {
		.owner    = THIS_MODULE ,
		.name     = "assoofs",
		.mount    = assoofs_mount ,
		.kill_sb = kill_litter_super ,
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


int assoofs_fill_super(struct super_block *sb, void *data , int silent)
{
	//Leer información del superbloque
	struct buffer_head *buff;
	struct assoofs_super_block_info *asb;
	struct inode *root_inode;

	buff=sb_bread(sb,ASSOOFS_SUPERBLOCK_BLOCK_NUMBER);//El super bloque está en el bloque 0
	asb=(struct assoofs_super_block_info *)buff->b_data;//No es el superbloque como tal, es infromación del superbloque.

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
	sb->s_fs_info=asb;//Guardamos el superbloque_info por si lo necesitamos luego.
	sb->s_op=&assoofs_sops;//direccion de variable tipo super_oepration que será global. Operaciones que soporta el superbloque

	//Crear el inodo raíz
	
	root_inode=new_inode(sb);
	inode_init_owner(root_inode,NULL,S_IFDIR);
	root_inode->i_ino=ASSOOFS_ROOTDIR_INODE_NUMBER;
	root_inode->i_atime=CURRENT_TIME;
	root_inode->i_mtime=CURRENT_TIME;
	root_inode->i_ctime=CURRENT_TIME;
	//root_inode->i_op=//3.2.4
	//root_inode->i_fop=//3.2.4
	//root_inode->i_private=//el inode_info, similar al asb en  s_fs_info
	d_make_root(root_inode);
	return 0;

}


module_init(assoofs_init);
module_exit(assoofs_exit);