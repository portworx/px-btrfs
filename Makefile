
obj-$(CONFIG_BTRFS_FS) := btrfs.o

btrfs-y += super.o ctree.o extent-tree.o print-tree.o root-tree.o dir-item.o \
	   file-item.o inode-item.o inode-map.o disk-io.o \
	   transaction.o inode.o file.o tree-defrag.o \
	   extent_map.o sysfs.o struct-funcs.o xattr.o ordered-data.o \
	   extent_io.o volumes.o async-thread.o ioctl.o locking.o orphan.o \
	   export.o tree-log.o free-space-cache.o zlib.o lzo.o \
	   compression.o delayed-ref.o relocation.o delayed-inode.o scrub.o \
	   reada.o backref.o ulist.o qgroup.o send.o dev-replace.o raid56.o \
	   uuid-tree.o props.o hash.o

btrfs-$(CONFIG_BTRFS_FS_POSIX_ACL) += acl.o
btrfs-$(CONFIG_BTRFS_FS_CHECK_INTEGRITY) += check-integrity.o

btrfs-$(CONFIG_BTRFS_FS_RUN_SANITY_TESTS) += tests/free-space-tests.o \
	tests/extent-buffer-tests.o tests/btrfs-tests.o \
	tests/extent-io-tests.o tests/inode-tests.o tests/qgroup-tests.o

KVERSION=$(shell uname -r)
ifndef KERNELPATH
     KERNELPATH=/usr/src/kernels/$(KVERSION)
endif 

ifeq ($(shell test -d $(KERNELPATH); echo $$?),1)
$(error Kernel path: $(KERNELPATH)  directory does not exist.)
endif 

SCRIPT="/home/px-btrfs/kernel_version.sh"
RHEL_VERSION_CODE:=$(shell $(SCRIPT) $(KERNELPATH))

ifneq ($(RHEL_VERSION_CODE), 0)
  ccflags-y += -DRHEL_VERSION_CODE=$(RHEL_VERSION_CODE)
endif

ifeq ($(shell test  -f "/usr/bin/bc"; echo $$?),0)
MINKVER=3.10
KERNELVER=$(shell echo $(KVERSION) | /bin/sed 's/-.*//' | /bin/sed 's/\(.*\..*\)\..*/\1/')
ifeq ($(shell echo "$(KERNELVER)>=$(MINKVER)" | /usr/bin/bc),0)
$(error Kernel version error: Build kernel version must be >= $(MINKVER).)
endif 
endif 

MAJOR=$(shell echo $(KVERSION) | awk -F. '{print $$1}')
MINOR=$(shell echo $(KVERSION) | awk -F. '{print $$2}')
PATCH=$(shell echo $(KVERSION) | awk -F. '{print $$3}' | awk -F- '{print $$1}')
export REVISION=$(shell echo $(KVERSION) | awk -F. '{print $$3}' |  awk -F- '{print $$2}')
export VERSION=$(MAJOR).$(MINOR).$(PATCH)
export KERNELPATH
export OUTPATH

.PHONY: rpm

all: 
	make -C $(KERNELPATH) M=$(CURDIR) modules

insert: all
	insmod btrfs.ko	
	
clean:
	make -C $(KERNELPATH) M=$(CURDIR) clean

install:
	make V=1 -C $(KERNELPATH) M=$(CURDIR) modules_install 
	
distclean: clean
	@/bin/rm -f  config.* Makefile
