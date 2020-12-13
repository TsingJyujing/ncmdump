# 网易云NCM文件转换器

## 简介

本项目基于 [anonymous5l/ncmdump](https://github.com/anonymous5l/ncmdump) 修改而来。
本来想小修一下提个PR，但是作者似乎不太合并PR，所以就大刀阔斧随便改了。

主要的改动是：

- 文件写入到与源文件相同的文件夹
- 支持扫描文件夹，生成所有的文件
- 顺手优化代码
    - 统一格式
    - 去除不使用的函数
    - 用了一些C++17的新特性

## 使用文档

对于单个文件，直接`ncmdump file <文件名>`即可，如果需要遍历文件夹，那就`ncmdump scan <文件夹>`

其余参数：

- `--verbose`: 决定日志等级，如果开启则为DEBUG级别
- `--rm`: 如果转换成功就删除原文件
- `--skip`: 跳过转换后文件已存在的任务

手册：

```
Usage: ncmdump [options] command path 

Positional arguments:
command         The command you'd like to run: [file|scan]
path            Path of the file/dir

Optional arguments:
-h --help       shows help message and exits
-v --version    prints version information and exits
--rm            Remove NCM file if it's successfully dumped.
--skip          Skip writing if converted file already existed.
--verbose       Print detail logs
```

## 参考资料

感谢所有的开源作者，让我的工作进行的非常容易。

- 原始项目地址：[Netease Cloud Music Copyright Protection File Dump](https://github.com/anonymous5l/ncmdump)
- 日志记录使用了 [gabime/spdlog](https://github.com/gabime/spdlog)
- 在解析参数的时候使用了 [p-ranav/argparse](https://github.com/p-ranav/argparse)
- 具体的原理拜读了 [网易云音乐ncm格式分析以及ncm与mp3格式转换](https://www.cnblogs.com/cyx-b/p/13443003.html) [archive](https://archive.vn/HTe43)

## 相关项目地址  


- Windows GUI版本 [ncmdump-gui](https://github.com/anonymous5l/ncmdump-gui)
- Windows GUI应用程序 [ncmdump-gui-release](https://github.com/anonymous5l/ncmdump-gui/releases/tag/fully) 
    - 运行库基于 `.NetFramework 4.6.1`
- Android版本 [DroidNCM](https://github.com/bunnyblueair/DroidNCM)

