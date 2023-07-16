import multiprocessing
import ffmpeg
import numpy as np
import cv2
from PIL import Image, ImageFont, ImageDraw
import socket

camera_channel = 0  # 选择主码流，还是辅码流
IP = "189.23.33.234"
RTSP_URL = f"rtsp://189.23.33.234:8554/"

def get_coordinate(coordiArray):
    s = socket.socket()
    s.bind((IP, 50010))  # 绑定端口
    s.listen()  # 开始监听
    chanel, client = s.accept()  # 返回通道和客户端信息
    while True:
        recive_content = chanel.recv(1024).decode()  # 通道获取内容（1024是缓冲区大小，意味着接收到数据的最大长度）,并进行解码，这就是里面的内容
        temp = recive_content.split()
        for idx in range(len(temp)):
            coordiArray[idx] = int(temp[idx])
    # 关闭连接（不过这一步到不了）
    s.close()
    # 获取Socket对象

def show_chinese(img,text,pos):
    """
    :param img: opencv 图片
    :param text: 显示的中文字体
    :param pos: 显示位置
    :return:    带有字体的显示图片（包含中文）
    """
    img_pil = Image.fromarray(cv2.cvtColor(img, cv2.COLOR_BGR2RGB))
    font = ImageFont.truetype(font='msyh.ttc', size=28)
    draw = ImageDraw.Draw(img_pil)
    draw.text(pos, text, font=font, fill=(255, 0, 0))  # PIL中RGB=(255,0,0)表示红色
    img_cv = np.array(img_pil)                         # PIL图片转换为numpy
    img = cv2.cvtColor(img_cv, cv2.COLOR_RGB2BGR)      # PIL格式转换为OpenCV的BGR格式
    return img

def show_rectangle(img):
    # 在图像上绘制一个矩形
    x, y, w, h = 980, -10, 400, 120  # 矩形左上角坐标和宽度、高度
    color = (0, 0, 255)  # 矩形颜色（BGR格式）
    thickness = 2  # 矩形边框宽度
    cv2.rectangle(img, (x, y), (x + w, y + h), color, thickness)


def main(coordiArray):
    args = {
        "rtsp_transport": "tcp",
        "fflags": "nobuffer",
        "flags": "low_delay"
    }    # 添加参数
    probe = ffmpeg.probe(RTSP_URL)
    cap_info = next(x for x in probe['streams'] if x['codec_type'] == 'video')
    print("fps: {}".format(cap_info['r_frame_rate']))
    width = cap_info['width']           # 获取视频流的宽度
    height = cap_info['height']         # 获取视频流的高度
    up, down = str(cap_info['r_frame_rate']).split('/')
    fps = eval(up) / eval(down)
    print("fps: {}".format(fps))    # 读取可能会出错错误
    process1 = (
        ffmpeg
        .input(RTSP_URL, **args)
        .output('pipe:', format='rawvideo', pix_fmt='rgb24')
        .overwrite_output()
        .run_async(pipe_stdout=True)
    )
    while True:
        in_bytes = process1.stdout.read(width * height * 3)     # 读取图片
        if not in_bytes:
            break
        # 转成ndarray
        in_frame = (
            np
            .frombuffer(in_bytes, np.uint8)
            .reshape([height, width, 3])
        )
        frame = cv2.resize(in_frame, (1280, 720))   # 改变图片尺寸
        frame = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)  # 转成BGR
        frame = show_chinese(frame, "  AI自主打击系统\n实时坐标：({}, {})".format(coordiArray[0], coordiArray[1]), (990, 20))
        show_rectangle(frame)
        cv2.imshow("ffmpeg", frame)
        if cv2.waitKey(1) == ord('q'):
            break
    process1.kill()             # 关闭


if __name__ == "__main__":
    manager = multiprocessing.Manager()
    shared_list = manager.list([1, 2])

    t1 = multiprocessing.Process(target=get_coordinate, args=(shared_list,))
    # 启动运行线程
    t1.start()
    main(shared_list)
