function listcode
% uf = dir('training_dataset/JPEGImages/*.png');
% fprintf("num:%d\n", length(uf));
uf = dir('test_dataset/JPEGImages/*.png');
fprintf("num:%d\n", length(uf));
for i = 1:length(uf)
    dot = strfind(uf(i).name,'.');
    imname = uf(i).name(1:dot-1); 
    fprintf("imname:%s\n", imname);
    im = imread(['test_dataset/JPEGImages/' uf(i).name]);
    imshow(im);
    filename = strcat(imname, '.png');
    fprintf("filename:%s\n", filename);
    jpg_dir = strcat('/home/nict02-s03/ai/sample/tank_detect/test_dataset/JPEGImages/', filename);
    fid=fopen('F:\嵌入式竞赛\tank_detect\tank_detect\test_dataset\list\tank_test.txt','a+'); %写入文件路径
    fprintf(fid, '%s\n', jpg_dir);
    fclose(fid);
%     disp('Press any key to move onto the next image');pause;
end