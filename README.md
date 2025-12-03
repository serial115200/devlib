# Devlib - CPM 包管理项目

这是一个使用 CPM (CMake Package Manager) 管理 C/C++ 库和包的项目。CPM 是一个轻量级的 CMake 包管理器，无需额外安装工具。

## 目录结构

```
devlib/
├── packages/          # 各种包的 CPM 配置
│   └── json-c/       # json-c 包配置
│       └── package.cmake
├── config/           # 配置文件，管理是否启用各个包
│   └── packages.json # 包启用配置
├── build/            # 编译输出目录
├── download/         # Git 仓库 clone 目录
├── rootfs/           # 包安装目录，所有包都会安装到这里
│   └── usr/          # 标准 Unix 目录结构
│       ├── lib/      # 库文件
│       ├── include/  # 头文件
│       └── bin/      # 可执行文件
├── cmake/            # CMake 辅助文件
│   ├── CPM.cmake     # CPM 包管理器（本地文件）
│   └── DevlibRootfs.cmake  # 自动配置 rootfs 路径
├── example/          # 示例项目
├── CMakeLists.txt    # 主 CMake 配置文件
├── build.sh          # 构建脚本
└── setup_proxy.sh    # 代理设置脚本
```

## 使用方法

### 1. 安装依赖

只需要 CMake（通常系统已安装）：

```bash
# 检查 CMake 版本（需要 3.15+）
cmake --version

# 如果没有，安装 CMake
sudo apt install cmake
```

**无需安装其他包管理器！** 项目已包含 CPM.cmake 文件，直接使用即可。

### 2. 配置代理（如果需要）

如果网络环境需要代理，可以设置代理环境变量：

**方法 1: 使用 HTTP/HTTPS 代理（推荐）**

```bash
export HTTP_PROXY="http://172.16.8.170:7890"
export HTTPS_PROXY="http://172.16.8.170:7890"
```

**方法 2: 使用 SOCKS5 代理**

```bash
export all_proxy="socks5h://172.16.8.170:7890"
# 然后运行代理设置脚本
source setup_proxy.sh
```

**注意**: CMake 的 `file(DOWNLOAD)` 和 `FetchContent` 主要支持 HTTP/HTTPS 代理。如果使用 SOCKS5 代理：
- 如果代理服务器同时提供 HTTP 代理，请使用 HTTP 代理地址
- 或者配置代理转换工具（如 proxychains）
- Git 下载可以通过 Git 配置使用 SOCKS5 代理

### 3. 配置包启用状态

编辑 `config/packages.json` 文件来启用或禁用包：

```json
{
  "json-c": {
    "enabled": true,
    "version": "0.18",
    "repo": "https://github.com/json-c/json-c.git",
    "hash": "json-c-0.18",
    "options": {
      "shared": false
    }
  }
}
```

**注意**: 包会通过 Git clone 到 `download/` 目录，而不是直接下载压缩包。

### 4. 构建项目

使用提供的构建脚本：

```bash
chmod +x build.sh
./build.sh
```

构建完成后，所有包都会安装到 `rootfs/usr` 目录中，采用标准的 Unix 目录结构：
- `rootfs/usr/lib/` - 库文件
- `rootfs/usr/include/` - 头文件
- `rootfs/usr/bin/` - 可执行文件

### 5. 手动构建（可选）

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX=../rootfs/usr
cmake --build . --config Release
cmake --install .
```

## json-c 包

json-c 是一个用 C 语言实现的 JSON 库。

### 包选项

- `shared`: 是否构建共享库（默认: false）

## 在项目中使用 rootfs 中的包

### 方法 1: 使用 DevlibRootfs.cmake（推荐）

```cmake
# 在你的 CMakeLists.txt 开头添加
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/DevlibRootfs.cmake)

# 然后直接链接库
target_link_libraries(your_target json-c)
target_include_directories(your_target PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../rootfs/usr/include)
```

### 方法 2: 手动配置路径

```cmake
# 设置 rootfs 路径
set(ROOTFS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../rootfs")

# 添加包含目录
include_directories(${ROOTFS_DIR}/usr/include)

# 添加库目录
link_directories(${ROOTFS_DIR}/usr/lib)

# 使用包
target_link_libraries(your_target json-c)
```

## 添加新包

1. 在 `packages/` 目录下创建新包的目录
2. 在新目录中创建 `package.cmake`，使用 CPMAddPackage 添加包
3. 在 `config/packages.json` 中添加配置
4. 在主 `CMakeLists.txt` 中包含包的配置

示例 `packages/newlib/package.cmake`:

```cmake
CPMAddPackage(
    NAME newlib
    VERSION 1.0.0
    URL https://example.com/newlib.tar.gz
    OPTIONS
        "BUILD_SHARED_LIBS OFF"
)
```

然后在 `CMakeLists.txt` 中添加：
```cmake
include(packages/newlib/package.cmake)
```

## CPM 的优势

- ✅ **无需额外安装**：只需要 CMake，CPM 会自动下载
- ✅ **简单直接**：所有配置都在 CMake 中
- ✅ **快速**：自动缓存下载的包
- ✅ **灵活**：可以控制包的版本、选项等
- ✅ **跨平台**：支持所有 CMake 支持的平台

## 代理配置说明

### 问题诊断

如果遇到下载失败，检查代理设置：

```bash
# 检查当前代理设置
env | grep -i proxy

# 测试代理是否工作
curl -I --proxy "$HTTP_PROXY" https://github.com
```

### 常见问题

1. **SOCKS5 代理不被支持**
   - CMake 主要支持 HTTP/HTTPS 代理
   - 解决方案：使用 HTTP 代理或配置代理转换

2. **Git 下载失败**
   - Git 需要单独配置代理
   - 解决方案：
     ```bash
     git config --global http.proxy http://172.16.8.170:7890
     git config --global https.proxy http://172.16.8.170:7890
     ```

3. **环境变量大小写问题**
   - 建议同时设置大小写版本
   - `HTTP_PROXY` 和 `http_proxy` 都设置

## 示例项目

查看 `example/` 目录中的示例代码，了解如何在项目中使用 rootfs 中的包。
