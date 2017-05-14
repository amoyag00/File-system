#include  <linux/module.h>         /*  Needed  by all  modules  */
#include  <linux/kernel.h>         /*  Needed  for  KERN_INFO   */
#include  <linux/init.h>           /*  Needed  for  the  macros  */
#include  <linux/fs.h>             /*  libfs  stuff             */
#include  <asm/uaccess.h>          /*  copy_to_user            */
#include  <linux/buffer_head.h>    /*  buffer_head              */
#include  <linux/slab.h>  


struct  file_system_type {
	const  char *name;
	int  fs_flags;
	#define  FS_REQUIRES_DEV    1
	#define  FS_BINARY_MOUNTDATA 2
	#define  FS_HAS_SUBTYPE     4
	#define  FS_USERNS_MOUNT    8 /* Can be  mounted  by  userns  root */
	#define  FS_RENAME_DOES_D_MOVE  32768 /* FS will  handle  d_move ()  during  rename ()  internally. */
	struct  dentry  *(* mount) (struct  file_system_type *, int ,
	const  char *, void *);
	void (* kill_sb) (struct  super_block  *);
	struct  module *owner;
	struct  file_system_type * next;
	struct  hlist_head  fs_supers;
	struct  lock_class_key  s_lock_key;
	struct  lock_class_key  s_umount_key;
	struct  lock_class_key  s_vfs_rename_key;
	struct  lock_class_key  s_writers_key[SB_FREEZE_LEVELS ];
	struct  lock_class_key  i_lock_key;
	struct  lock_class_key  i_mutex_key;
	struct  lock_class_key  i_mutex_dir_key;
};


MODULE_LICENSE("GPL");

MODULE_AUTHOR("Angel  Manuel  Guerrero  Higueras");

static  struct  file_system_type  assoofs_type = {
		.owner    = THIS_MODULE ,
		.name     = "assoofs",
		.mount    = assoofs_mount ,
		.kill_sb = kill_litter_super ,
	};


static  int  __init  init_module(void)
{

	 register_filesystem(&assoofs_type);
	return  0;
}


static  void  __exit  cleanup_module(void)
{
	 unregister_filesystem(&assoofs_type);
}


static  struct  dentry *assoofs_mount(struct  file_system_type *fs_type ,
int flags , const  char *dev_name , void *data)
{
	mount_bdev(struct  file_system_type *fs_type ,
	int flags , const  char *dev_name , void *data ,
	int (* fill_super)(struct  super_block *, void *, int));
}

int assoofs_fill_super(struct super_block *sb, void *data , int silent)
{
	
}
module_init(init_module);
module_exit(cleanup_module);