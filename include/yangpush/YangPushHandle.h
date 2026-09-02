//
// Copyright (c) 2019-2025 yanggaofeng
//
#ifndef INCLUDE_YANGPUSH_YANGPUSHHANDLE_H_
#define INCLUDE_YANGPUSH_YANGPUSHHANDLE_H_

#include <yangutil/buffer/YangVideoBuffer.h>
#include <yangutil/yangavinfotype.h>
#include <yangutil/sys/YangSysMessageI.h>
#include <yangpush/YangPushCommon.h>
#include <yangpush/YangSendVideoI.h>

class YangPushHandle {
public:
    YangPushHandle() = default;
    virtual ~YangPushHandle() = default;

public:
    virtual int publish(const char* url, yangbool isWhip) = 0;
    virtual void disconnect() = 0;

    virtual void init() = 0;
    virtual void changeSrc(int videoType) = 0;

    virtual YangVideoBuffer* getPreVideoBuffer() = 0;
};

#endif // INCLUDE_YANGPUSH_YANGPUSHHANDLE_H_
