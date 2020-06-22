//
// Created by xinyang on 2020/6/3.
//

#ifndef TOS_CONTAINER_H
#define TOS_CONTAINER_H

namespace tOS {
    // 空类型，作为占位符代替void。
    // 占用一个字节，略有性能损失。
    class Empty {
    };

    // 消息容器枚举
    enum ContainerEnum {
        CIRCULAR_QUEUE, STACK
    };
}

#endif /* TOS_CONTAINER_H */
