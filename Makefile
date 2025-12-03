.PHONY: clean cleanall cleanrootfs cleandownload help

# 目录定义
BUILD_DIR := build
ROOTFS_DIR := rootfs
DOWNLOAD_DIR := download

# 默认目标
help:
	@echo "可用目标:"
	@echo "  make clean        - 清理构建目录 (build/)"
	@echo "  make cleanrootfs  - 清理安装目录 (rootfs/)"
	@echo "  make cleandownload - 清理下载目录 (download/)"
	@echo "  make cleanall     - 清理所有目录 (build/, rootfs/, download/)"

# 清理构建目录
clean:
	@echo "清理构建目录: $(BUILD_DIR)/"
	@if [ -d "$(BUILD_DIR)" ]; then \
		rm -rf $(BUILD_DIR); \
		echo "✓ 已清理 $(BUILD_DIR)/"; \
	else \
		echo "✓ $(BUILD_DIR)/ 不存在，无需清理"; \
	fi

# 清理安装目录
cleanrootfs:
	@echo "清理安装目录: $(ROOTFS_DIR)/"
	@if [ -d "$(ROOTFS_DIR)" ]; then \
		rm -rf $(ROOTFS_DIR); \
		echo "✓ 已清理 $(ROOTFS_DIR)/"; \
	else \
		echo "✓ $(ROOTFS_DIR)/ 不存在，无需清理"; \
	fi

# 清理下载目录
cleandownload:
	@echo "清理下载目录: $(DOWNLOAD_DIR)/"
	@if [ -d "$(DOWNLOAD_DIR)" ]; then \
		rm -rf $(DOWNLOAD_DIR); \
		echo "✓ 已清理 $(DOWNLOAD_DIR)/"; \
	else \
		echo "✓ $(DOWNLOAD_DIR)/ 不存在，无需清理"; \
	fi

# 清理所有
cleanall: clean cleanrootfs cleandownload
	@echo ""
	@echo "✓ 所有目录已清理完成"

