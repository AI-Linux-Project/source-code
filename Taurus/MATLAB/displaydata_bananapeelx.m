function displaydata_bananapeelx
% 将labels文件夹里的txt文件坐标读取还原，用来画框看看标注是否正确
% Class id    center_x    center_y    w    h
% 对数据格式解释如下：
% Class id:表示标注框的类别，从0开始计算，当前只要手部1类检测物体，故Class id全为0；
% center_x:表示归一化后的手部框中心点坐标的X值。归一化坐标 = 实际坐标 / 整个图片宽
% center_y:表示归一化后的手部框中心点坐标的Y值。归一化坐标 = 实际坐标 / 整个图片高
% w:表示归一化后的手部框的宽。归一化长度 = 实际长度 / 整个图片宽
% h:表示归一化后的手部框的高。归一化长度 = 实际长度 /整个图片高

% uf = dir('images/training_data/*.png');
% files = dir('labels/train_labels/*.txt');        % 获取.labels文件夹下的所有.txt文件
uf = dir('images/test_data/*.png');
files = dir('labels/test_labels/*.txt');
for i = 1:length(uf)
    dot = strfind(uf(i).name,'.');
    imname = uf(i).name(1:dot-1); 
    fprintf("imname:%s\n", imname);
    filename = files(i).name;
    fprintf("filename:%s\n", filename);
    %%% 读取.txt文件内容
   % data = dlmread(['labels/train_labels/' filename]);
    data = dlmread(['labels/test_labels/' filename]);
    % 提取数据
    center_x = data(:, 2);
    center_y = data(:, 3);
    w = data(:, 4);
    h = data(:, 5);
     % 获取行数
    num_lines = size(data, 1);
    %%%显示图片
    %im = imread(['images/training_data/' uf(i).name]);
    im = imread(['images/test_data/' uf(i).name]);
    imshow(im);
    Width = size(im, 2);
    Height = size(im, 1);
    fprintf("Width_test:%d, height_test:%d\n", Width, Height);
    %%画框
    for j = 1:num_lines
        x=center_x*Width;  long=w*Width;
        y=center_y*Height;  high=h*Height;
        Vec_max_x = x+long/2;
        Vec_min_x = x-long/2;
        Vec_max_y = y+high/2;
        Vec_min_y = y-high/2;
        fprintf("Vec_max_x:%g,Vec_min_x:%g\n", Vec_max_x, Vec_min_x);
        fprintf("Vec_max_y:%g,Vec_min_y:%g\n", Vec_max_y, Vec_min_y);
        line([Vec_min_x Vec_min_x]',[Vec_min_y Vec_max_y]','LineWidth',3,'Color','b');
        line([Vec_min_x Vec_max_x]',[Vec_max_y Vec_max_y]','LineWidth',3,'Color','b');
        line([Vec_max_x Vec_max_x]',[Vec_max_y Vec_min_y]','LineWidth',3,'Color','b');
        line([Vec_max_x Vec_min_x]',[Vec_min_y Vec_min_y]','LineWidth',3,'Color','b');       
    end
    fprintf("i:%d\n",i);
    disp('Press any key to move onto the next image');pause;
end