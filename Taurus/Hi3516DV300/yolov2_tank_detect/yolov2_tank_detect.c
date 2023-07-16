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

/*
 * 该文件提供了基于yolov2的手部检测以及基于resnet18的手势识别，属于两个wk串行推理。
 * 该文件提供了手部检测和手势识别的模型加载、模型卸载、模型推理以及AI flag业务处理的API接口。
 * 若一帧图像中出现多个手，我们通过算法将最大手作为目标手送分类网进行推理，
 * 并将目标手标记为绿色，其他手标记为红色。
 *
 * This file provides hand detection based on yolov2 and gesture recognition based on resnet18,
 * which belongs to two wk serial inferences. This file provides API interfaces for model loading,
 * model unloading, model reasoning, and AI flag business processing for hand detection
 * and gesture recognition. If there are multiple hands in one frame of image,
 * we use the algorithm to use the largest hand as the target hand for inference,
 * and mark the target hand as green and the other hands as red.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "sample_comm_nnie.h"
#include "sample_media_ai.h"
#include "ai_infer_process.h"
#include "yolov2_tank_detect.h"
#include "vgs_img.h"
#include "ive_img.h"
#include "misc_util.h"
#include "hisignalling.h"

// #include "tennis_detect.h"
#include "base_interface.h"
#include "posix_help.h"
//#include "sample_media_opencv.h"

// #include <opencv2/core.hpp>
// #include <opencv2/highgui.hpp>
// #include <opencv2/imgproc.hpp>

#include "sample_comm_ive.h"
#include "misc_util.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

#define HAND_FRM_WIDTH     640
#define HAND_FRM_HEIGHT    384
#define DETECT_OBJ_MAX     32
#define RET_NUM_MAX        4
#define DRAW_RETC_THICK    8    // Draw the width of the line
#define WIDTH_LIMIT        32
#define HEIGHT_LIMIT       32
#define IMAGE_WIDTH        224  // The resolution of the model IMAGE sent to the classification is 224*224
#define IMAGE_HEIGHT       224
#define MODEL_FILE_TANK    "/userdata/models/tank_classify/tank_detect.wk" // darknet framework wk model

#define PIRIOD_NUM_MAX     49 // Logs are printed when the number of targets is detected
#define DETECT_OBJ_MAX     32 // detect max obj
static int biggestBoxIndex;
static IVE_IMAGE_S img;
static DetectObjInfo objs[DETECT_OBJ_MAX] = {0};
static RectBox boxs[DETECT_OBJ_MAX] = {0};
static RectBox objBoxs[DETECT_OBJ_MAX] = {0};
static RectBox remainingBoxs[DETECT_OBJ_MAX] = {0};
static RectBox cnnBoxs[DETECT_OBJ_MAX] = {0}; // Store the results of the classification network
static RecogNumInfo numInfo[RET_NUM_MAX] = {0};
static IVE_IMAGE_S imgIn;
static IVE_IMAGE_S imgDst;
static VIDEO_FRAME_INFO_S frmIn;
static VIDEO_FRAME_INFO_S frmDst;
int uartFd_1 = 0;

/*  banana-peel-x修改
 * 加载tank检测模型
 * Load tank detect model
 */
HI_S32 Yolo2TankDetectResnetClassifyLoad(uintptr_t* model)
{
    SAMPLE_SVP_NNIE_CFG_S *self = NULL;
    HI_S32 ret;

    ret = CnnCreate(&self, MODEL_FILE_TANK);
    *model = ret < 0 ? 0 : (uintptr_t)self;
    TankDetectInit(); // Initialize the hand detection model
    SAMPLE_PRT("Load Tank detect  model success\n");
    /*
     * Uart串口初始化
     * Uart open init
     */
    uartFd_1 = UartOpenInit();
    if (uartFd_1 < 0) {
        printf("uart1 open failed\r\n");
    } else {
        printf("uart1 open successed\r\n");
    }
    return ret;
}

/*
 * 卸载Tank检测
 * Unload hand detect and classify model
 */
HI_S32 Yolo2TankDetectResnetClassifyUnload(uintptr_t model)
{
    CnnDestroy((SAMPLE_SVP_NNIE_CFG_S*)model);
    TankDetectExit(); // Uninitialize the hand detection model
    close(uartFd_1);
    SAMPLE_PRT("Unload tank detect  model success\n");

    return 0;
}

/*
 * 获得最大的手
 * Get the maximum hand
 */
static HI_S32 GetBiggestHandIndex(RectBox boxs[], int detectNum)
{
    HI_S32 handIndex = 0;
    HI_S32 biggestBoxIndex = handIndex;
    HI_S32 biggestBoxWidth = boxs[handIndex].xmax - boxs[handIndex].xmin + 1;
    HI_S32 biggestBoxHeight = boxs[handIndex].ymax - boxs[handIndex].ymin + 1;
    HI_S32 biggestBoxArea = biggestBoxWidth * biggestBoxHeight;

    for (handIndex = 1; handIndex < detectNum; handIndex++) {
        HI_S32 boxWidth = boxs[handIndex].xmax - boxs[handIndex].xmin + 1;
        HI_S32 boxHeight = boxs[handIndex].ymax - boxs[handIndex].ymin + 1;
        HI_S32 boxArea = boxWidth * boxHeight;
        if (biggestBoxArea < boxArea) {
            biggestBoxArea = boxArea;
            biggestBoxIndex = handIndex;
        }
        biggestBoxWidth = boxs[biggestBoxIndex].xmax - boxs[biggestBoxIndex].xmin + 1;
        biggestBoxHeight = boxs[biggestBoxIndex].ymax - boxs[biggestBoxIndex].ymin + 1;
    }

    if ((biggestBoxWidth == 1) || (biggestBoxHeight == 1) || (detectNum == 0)) {
        biggestBoxIndex = -1;
    }

    return biggestBoxIndex;
}

/*
 * 手势识别信息
 * Hand gesture recognition info
 */
static void HandDetectFlag(const RecogNumInfo resBuf)
{
    HI_CHAR *gestureName = NULL;
    switch (resBuf.num) {
        case 0u:
            gestureName = "gesture fist";
            UartSendRead(uartFd_1, FistGesture); // 拳头手势
            SAMPLE_PRT("----gesture name----:%s\n", gestureName);
            break;
        case 1u:
            gestureName = "gesture indexUp";
            UartSendRead(uartFd_1, ForefingerGesture); // 食指手势
            SAMPLE_PRT("----gesture name----:%s\n", gestureName);
            break;
        case 2u:
            gestureName = "gesture OK";
            UartSendRead(uartFd_1, OkGesture); // OK手势
            SAMPLE_PRT("----gesture name----:%s\n", gestureName);
            break;
        case 3u:
            gestureName = "gesture palm";
            UartSendRead(uartFd_1, PalmGesture); // 手掌手势
            SAMPLE_PRT("----gesture name----:%s\n", gestureName);
            break;
        case 4u:
            gestureName = "gesture yes";
            UartSendRead(uartFd_1, YesGesture); // yes手势
            SAMPLE_PRT("----gesture name----:%s\n", gestureName);
            break;
        case 5u:
            gestureName = "gesture pinchOpen";
            UartSendRead(uartFd_1, ForefingerAndThumbGesture); // 食指 + 大拇指
            SAMPLE_PRT("----gesture name----:%s\n", gestureName);
            break;
        case 6u:
            gestureName = "gesture phoneCall";
            UartSendRead(uartFd_1, LittleFingerAndThumbGesture); // 大拇指 + 小拇指
            SAMPLE_PRT("----gesture name----:%s\n", gestureName);
            break;
        default:
            gestureName = "gesture others";
            UartSendRead(uartFd_1, InvalidGesture); // 无效值
            SAMPLE_PRT("----gesture name----:%s\n", gestureName);
            break;
    }
    SAMPLE_PRT("hand gesture success\n");
}
/*
Tank 中心数据传送
*/
static void TankCenterDataSend(int x ,int y)
{
    UartSendRead_Tank(uartFd_1, x,y); 
    // char *coordinate = NULL;
    // coordinate = "100 200"; 

    // sprintf(coordinate, "%d %d" , x,y);
    // UartSend(uartFd_1, (unsigned char*)coordinate, strlen(coordinate));
}
/*
 * Tank检测
 * Hand detect and classify calculation
 */
static AiPlugLib g_tennisWorkPlug = {0};
HI_S32 Yolo2TankDetectResnetClassifyCal(uintptr_t model, VIDEO_FRAME_INFO_S *srcFrm, VIDEO_FRAME_INFO_S *dstFrm)
{
    SAMPLE_SVP_NNIE_CFG_S *self = (SAMPLE_SVP_NNIE_CFG_S*)model;
    HI_S32 resLen = 0;
    int objNum;
    int ret;
    int num = 0;
    TankCenter *tankcenter;
    int x=0,y=0;
    ret = FrmToOrigImg((VIDEO_FRAME_INFO_S*)srcFrm, &img);
    SAMPLE_CHECK_EXPR_RET(ret != HI_SUCCESS, ret, "tank detect for YUV Frm to Img FAIL, ret=%#x\n", ret);

    objNum = TankDetectCal(&img, objs); // Send IMG to the detection net for reasoning
    for (int i = 0; i < objNum; i++) {
        cnnBoxs[i] = objs[i].box;
        RectBox *box = &objs[i].box;
        RectBoxTran(box, HAND_FRM_WIDTH, HAND_FRM_HEIGHT,
            dstFrm->stVFrame.u32Width, dstFrm->stVFrame.u32Height);
        SAMPLE_PRT("yolo2_out: {%d, %d, %d, %d}\n", box->xmin, box->ymin, box->xmax, box->ymax);
        boxs[i] = *box;
    }
    biggestBoxIndex = GetBiggestHandIndex(boxs, objNum);
    SAMPLE_PRT("biggestBoxIndex:%d, objNum:%d\n", biggestBoxIndex, objNum);

    /*
     * 当检测到对象时，在DSTFRM中绘制一个矩形
     * When an object is detected, a rectangle is drawn in the DSTFRM
     */
    if (biggestBoxIndex >= 0) {

        /*开启一个线程，执行绿点识别，识别之后进行同步继续往下执行，把识别后的框直接加到原图像上*/
        // int ret1;
        // tennis_detect opencv;
        // if (GetCfgBool("tennis_detect_switch:support_tennis_detect", true)) {
        //     if (g_tennisWorkPlug.model == 0) {
        //         ret1 = opencv.TennisDetectLoad(&g_tennisWorkPlug.model);
        //         if (ret < 0) {
        //             g_tennisWorkPlug.model = 0;
        //            // SAMPLE_CHECK_EXPR_GOTO(ret1 < 0, TENNIS_RELEASE, "TennisDetectLoad err, ret=%#x\n", ret);
        //         }
        //     }

        //     VIDEO_FRAME_INFO_S calFrm;
        //     ret1 = MppFrmResize((VIDEO_FRAME_INFO_S*)srcFrm, &calFrm, 640, 480); // 640: FRM_WIDTH, 480: FRM_HEIGHT
        //     ret1 = opencv.TennisDetectCal(g_tennisWorkPlug.model, &calFrm, (VIDEO_FRAME_INFO_S*)srcFrm);
          //  SAMPLE_CHECK_EXPR_GOTO(ret1 < 0, TENNIS_RELEASE, "TennisDetectCal err, ret=%#x\n", ret1);

            // ret1 = HI_MPI_VO_SendFrame(voLayer, voChn, &frm, 0);
            // SAMPLE_CHECK_EXPR_GOTO(ret1 != HI_SUCCESS, TENNIS_RELEASE,
            //     "HI_MPI_VO_SendFrame err, ret=%#x\n", ret1);
            // MppFrmDestroy(&calFrm);
        // }

        objBoxs[0] = boxs[biggestBoxIndex];
        MppFrmDrawRects(dstFrm, objBoxs, 1, RGB888_RED, DRAW_RETC_THICK); // Target hand objnum is equal to 1

        x =  (int)(objBoxs[0].xmax+objBoxs[0].xmin)/2;
        y =  (int)(objBoxs[0].ymax+objBoxs[0].ymin)/2;
        
        TankCenterDataSend(x , y);
        SAMPLE_PRT("tankcenterx:%d, y:%d\n",x, y); 
        for (int j = 0; (j < objNum) && (objNum > 1); j++) {
            if (j != biggestBoxIndex) {
                remainingBoxs[num++] = boxs[j];
                /*
                 * 其他手objnum等于objnum -1
                 * Others hand objnum is equal to objnum -1
                 */
                MppFrmDrawRects(dstFrm, remainingBoxs, objNum - 1, RGB888_GREEN, DRAW_RETC_THICK);
            }
        }

        /*
         * 裁剪出来的图像通过预处理送分类网进行推理
         * The cropped image is preprocessed and sent to the classification network for inference
         */
        // ret = ImgYuvCrop(&img, &imgIn, &cnnBoxs[biggestBoxIndex]);
        // SAMPLE_CHECK_EXPR_RET(ret < 0, ret, "ImgYuvCrop FAIL, ret=%#x\n", ret);

        // if ((imgIn.u32Width >= WIDTH_LIMIT) && (imgIn.u32Height >= HEIGHT_LIMIT)) {
        //     COMPRESS_MODE_E enCompressMode = srcFrm->stVFrame.enCompressMode;
        //     ret = OrigImgToFrm(&imgIn, &frmIn);
        //     frmIn.stVFrame.enCompressMode = enCompressMode;
        //     SAMPLE_PRT("crop u32Width = %d, img.u32Height = %d\n", imgIn.u32Width, imgIn.u32Height);
        //     ret = MppFrmResize(&frmIn, &frmDst, IMAGE_WIDTH, IMAGE_HEIGHT);
        //     ret = FrmToOrigImg(&frmDst, &imgDst);
        //     ret = CnnCalImg(self,  &imgDst, numInfo, sizeof(numInfo) / sizeof((numInfo)[0]), &resLen);
        //     SAMPLE_CHECK_EXPR_RET(ret < 0, ret, "CnnCalImg FAIL, ret=%#x\n", ret);
        //     HI_ASSERT(resLen <= sizeof(numInfo) / sizeof(numInfo[0]));
        //     HandDetectFlag(numInfo[0]);
        //     MppFrmDestroy(&frmDst);
        // }
        // IveImgDestroy(&imgIn);




    }

    return ret;
}



static uintptr_t g_handModel = 0;

static HI_S32 Yolo2FdLoad(uintptr_t* model)
{
    SAMPLE_SVP_NNIE_CFG_S *self = NULL;
    HI_S32 ret;

    ret = Yolo2Create(&self, MODEL_FILE_TANK);
    *model = ret < 0 ? 0 : (uintptr_t)self;
    SAMPLE_PRT("Yolo2FdLoad ret:%d\n", ret);

    return ret;
}

HI_S32 TankDetectInit()
{
    return Yolo2FdLoad(&g_handModel);
}

static HI_S32 Yolo2FdUnload(uintptr_t model)
{
    Yolo2Destory((SAMPLE_SVP_NNIE_CFG_S*)model);
    return 0;
}

HI_S32 TankDetectExit()
{
    return Yolo2FdUnload(g_handModel);
}

static HI_S32 TankDetect(uintptr_t model, IVE_IMAGE_S *srcYuv, DetectObjInfo boxs[])
{
    SAMPLE_SVP_NNIE_CFG_S *self = (SAMPLE_SVP_NNIE_CFG_S*)model;
    int objNum;
    int ret = Yolo2CalImg(self, srcYuv, boxs, DETECT_OBJ_MAX, &objNum);
    if (ret < 0) {
        SAMPLE_PRT("Tank detect Yolo2CalImg FAIL, for cal FAIL, ret:%d\n", ret);
        return ret;
    }

    return objNum;
}

HI_S32 TankDetectCal(IVE_IMAGE_S *srcYuv, DetectObjInfo resArr[])
{
    int ret = TankDetect(g_handModel, srcYuv, resArr);
    return ret;
}



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
