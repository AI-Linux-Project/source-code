/*
 * Copyright (c) 2022 HiSilicon (Shanghai) Technologies CO., LIMITED.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef YOLOV2_TANK_DETECT_H
#define YOLOV2_TANK_DETECT_H

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "hi_comm_video.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */
/*
 * 加载手部检测和手势分类模型
 * Load hand detect and classify model
 */
HI_S32 Yolo2TankDetectResnetClassifyLoad(uintptr_t* model);

/*
 * 卸载手部检测和手势分类模型
 * Unload hand detect and classify model
 */
HI_S32 Yolo2TankDetectResnetClassifyUnload(uintptr_t model);

/*
 * 手部检测和手势分类推理
 * Hand detect and classify calculation
 */
HI_S32 Yolo2TankDetectResnetClassifyCal(uintptr_t model, VIDEO_FRAME_INFO_S *srcFrm, VIDEO_FRAME_INFO_S *dstFrm);

HI_S32 TankDetectInit();
HI_S32 TankDetectExit();
HI_S32 TankDetectCal(IVE_IMAGE_S *srcYuv, DetectObjInfo resArr[]);

static void TankCenterDataSend(int x ,int y);


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
#endif
