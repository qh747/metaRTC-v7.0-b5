//
// Copyright (c) 2019-2025 yanggaofeng
//

#ifndef INCLUDE_YANGUTIL_YANGFRAMEBUFFER_H_
#define INCLUDE_YANGUTIL_YANGFRAMEBUFFER_H_

#include <yangutil/yangavinfo.h>

#ifdef __cplusplus
extern "C"{
#endif

void yang_frame_copy(YangFrame* src, YangFrame* dst);
void yang_frame_copy_buffer(YangFrame* src, YangFrame* dst);
void yang_frame_copy_nobuffer(YangFrame* src, YangFrame* dst);

#ifdef __cplusplus
}
#endif

#endif // INCLUDE_YANGUTIL_YANGFRAMEBUFFER_H_
