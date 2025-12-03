.PHONY: clean cleanall cleanrootfs cleandownload help

# 目录定义
BUILD_DIR := build
ROOTFS_DIR := rootfs
DOWNLOAD_DIR := download

help:
	@echo "可用目标:"
	@echo "  make clean        	- 清理构建目录 (build/)"
	@echo "  make cleanrootfs  	- 清理安装目录 (rootfs/)"
	@echo "  make cleandl 		- 清理下载目录 (download/)"
	@echo "  make cleanall     	- 清理所有目录 (build/, rootfs/, download/)"

all:
	./build.sh

clean:
	rm -rf $(BUILD_DIR)
	rm -rf $(ROOTFS_DIR)

cleandl:
	rm -rf $(DOWNLOAD_DIR)

cleanall: clean cleanrootfs cleandl
	@echo ""
	@echo "✓ 所有目录已清理完成"
