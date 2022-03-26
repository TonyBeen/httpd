### httpd 一个不入流的http服务器

> ##### `暂未完善`
> `数据库池还未想好如何设计`

> ###### `服务器包含epoll，线程池(pthread)，协程(ucontext), 数据库池(mysql)，日志(alias/log)，守护进程等`
> ###### `epoll本意采用单线程监听，多线程处理client socket，然现在是集监听及处理于一身`
> ###### `epoll 接收到事件后将任务加入线程池，由其余线程做处理`
> ###### `虽包含了协程，但使用协程时仍按回调使用，因未想出可以更高效的使用逻辑`
> ###### `配置统一由config/config.yaml来管理，由Config加载，支持字符串转int, long, uint, ulong, bool, const char *; 未使用boost::lexical_cast`

> ###### 目前可以编译生成httpd，也可以获取到静态html，登录。但注册等一些基本逻辑暂未实现

> ###### TODO
> `1、过滤 User-Agent`

> ##### 改善
> 最想做的事是把技术和业务实现分离，无奈技术不到位，无法做到