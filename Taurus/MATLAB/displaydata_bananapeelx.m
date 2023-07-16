function displaydata_bananapeelx
% ��labels�ļ������txt�ļ������ȡ��ԭ���������򿴿���ע�Ƿ���ȷ
% Class id    center_x    center_y    w    h
% �����ݸ�ʽ�������£�
% Class id:��ʾ��ע�����𣬴�0��ʼ���㣬��ǰֻҪ�ֲ�1�������壬��Class idȫΪ0��
% center_x:��ʾ��һ������ֲ������ĵ������Xֵ����һ������ = ʵ������ / ����ͼƬ��
% center_y:��ʾ��һ������ֲ������ĵ������Yֵ����һ������ = ʵ������ / ����ͼƬ��
% w:��ʾ��һ������ֲ���Ŀ���һ������ = ʵ�ʳ��� / ����ͼƬ��
% h:��ʾ��һ������ֲ���ĸߡ���һ������ = ʵ�ʳ��� /����ͼƬ��

% uf = dir('images/training_data/*.png');
% files = dir('labels/train_labels/*.txt');        % ��ȡ.labels�ļ����µ�����.txt�ļ�
uf = dir('images/test_data/*.png');
files = dir('labels/test_labels/*.txt');
for i = 1:length(uf)
    dot = strfind(uf(i).name,'.');
    imname = uf(i).name(1:dot-1); 
    fprintf("imname:%s\n", imname);
    filename = files(i).name;
    fprintf("filename:%s\n", filename);
    %%% ��ȡ.txt�ļ�����
   % data = dlmread(['labels/train_labels/' filename]);
    data = dlmread(['labels/test_labels/' filename]);
    % ��ȡ����
    center_x = data(:, 2);
    center_y = data(:, 3);
    w = data(:, 4);
    h = data(:, 5);
     % ��ȡ����
    num_lines = size(data, 1);
    %%%��ʾͼƬ
    %im = imread(['images/training_data/' uf(i).name]);
    im = imread(['images/test_data/' uf(i).name]);
    imshow(im);
    Width = size(im, 2);
    Height = size(im, 1);
    fprintf("Width_test:%d, height_test:%d\n", Width, Height);
    %%����
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