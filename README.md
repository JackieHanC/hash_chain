#HASH_CHAIN

目录内含

1. ```
   hash_chain.cpp 源文件
   hash_chain     可执行文件(macOS下编译得出)
   README
   ```



源文件编译方式: `g++ -lcrypto hash_chain.cpp -o hash_chain`

命令行中`./hash_chain`可得help文档如下

```
Usage:
hash_chain create [string-to-hash] [hash-chain-filename]
hash_chain add [string-to-add-to-hash-chain] [existing-hash-chain-file] [new-hash-chain-file]
hash_chain verify [existing-hash-chain-file]
hash_chain print [existing-hash-chain-file]
```

hash chain中的每个节点含有上个节点的hash值，当前节点数据长度，当前节点数据，下个节点地址等信息。

程序可将hash chain转变为文件存储，暂时仅支持数据为字符串格式，不支持以文件为内容输入。