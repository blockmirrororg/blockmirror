#pragma once

/**
 * 网络协议大概设计框架
 * 1. 广播区块消息(BROADCAST_BLOCK)
 *   实际内容是区块的HASH32值 和 区块的高度
 *   将该HASH32和高度添加到区块的已知区块SET中 该SET建议以高度排序 限制大小
 *   收到该消息后首先查看blockstore中是否存在该区块 存在则不响应
 *   否则向对方发送 GET_BLOCK 消息
 * 
 * 2. 获取区块详情(GET_BLOCK) => (BLOCK_DETAIL)
 * 
 * 3. 获取交易
 *   1) 
 * 
 * 4. 获取数据
 * 
 * 1. 当出块节点产生区块后 会对所有P2P连接 广播该区块 HASH
 * 2. 
 * 
 */