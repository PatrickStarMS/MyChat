# chat
# muduo网络库安装
```bash
# 安装所需依赖
$ sudo apt install g++ cmake make libboost-dev

# 克隆 muduo 仓库
$ git clone -b cpp11 git@github.com:chenshuo/muduo.git
$ cd muduo

# 修改 CMake 配置，注释掉不需要编译的样例部分
$ vim CMakeLists.txt
# option(MUDUO_BUILD_EXAMPLES "Build Muduo examples" ON) 注释该行，不编译样例

# 编译 muduo
$ ./build.sh

# 安装 muduo
$ ./build.sh install  # muduo 编译后头文件和库文件都不在系统路径下

# 将 muduo 头文件拷贝到 mprpc 的 include 目录中
$ cd build/release-install-cpp11/include
$ cp -r muduo [your path]/mprpc/include/

# 将静态库文件拷贝到 mprpc 的 thirdparty 目录中
$ cd ../lib
$ cp libmuduo_base.a [your path]/mprpc/thirdparty/
$ cp libmuduo_net.a [your path]/mprpc/thirdparty/
```
# MySQL安装
```bash
$ sudo apt install mysql-server
$ sudo apt-get install libmysqlclient-dev
```
# Nginx安装
```bash
$ sudo apt-get install nginx
```
# Redis安装
```bash
$ sudo apt-get install redis-server
```
# hiredis安装
```bash
$ wget https://github.com/redis/hiredis/archive/v0.14.0.tar.gz
$ tar -xzf v0.14.0.tar.gz
$ cd hiredis-0.14.0/
$ make
$ sudo make install
$ sudo cp /usr/local/lib/libhiredis.so.0.14 /usr/lib/ # 将动态库移动到系统目录下
```

# Nginx均衡负载（TCP）配置
```bash
$ cd /etc/nginx/
$ vim nginx.conf
stream {
        upstream MyServer {
                server 127.0.0.1:6000 weight=1 max_fails=3 fail_timeout=30s;
                server 127.0.0.1:6002 weight=1 max_fails=3 fail_timeout=30s;
        }

        server {
                proxy_connect_timeout 1s;
                proxy_timeout 3s;
                listen 8000;
                proxy_pass MyServer;
                tcp_nodelay on;
        }
}
$ cd /usr/sbin/
$ ./nginx -s reload # 平滑重启
```
# 编译
```bash
$ cmake -B build
$ cmake --build build
```
