/**
 * 整个链的状态 代表当前区块时该链的状态
 * 1. account_store 存储账户以及余额
 * 2. block_store 存储最近可回滚区块<保存所有区块在硬盘>
 * 3. data_store 存储所有的最新数据
 * 4. format_store 存储所存在的数据格式
 * 5. transaction_store 存储所有未过期的交易(无论是否打包过)
 * 6. bps_store 存储现在的BP节点
 * 所有的存储都是关闭时才保存到文件中 打开时从文件中加载
 * 将来视情况可将部分表用映射文件的办法来实现
 */