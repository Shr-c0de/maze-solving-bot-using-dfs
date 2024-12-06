%% Clean the Workspace and command window
clear all;
clc;
close all;
%% Read the image of maze
Maze = imread('m5.png'); % Read your image here.
disp(size(Maze))
figure,imshow(Maze);title('Original Maze image');
%% Convert to binary image
if size(Maze,3) ==3
    Maze = rgb2gray(Maze);
end
Maze_Binary = imbinarize(Maze,graythresh(Maze)-0.1); % Make sure to have black walls and white path 
% There should not be any broken walls, walls should be seperate rom boundary of images  
disp(size(Maze_Binary))
figure,imshow(Maze_Binary);title('Binary Maze');
%% Start Solving 
%Use Watershed transform to find catchment basins
%Some other methods also can be used
Maze_Watershed = watershed(Maze_Binary);
C1 = (Maze_Watershed == 2);%Obtain First Part of Watershed
Maze_watershed1 = C1.*Maze_Binary;
figure,imshow(Maze_watershed1);title('One of the Part of catchment basins');
C2 = watershed(Maze_watershed1);
%Using watershed transform once again so the image obtained will be
%shrinked along path of maze, imshow pir is used to observe this.
figure,imshowpair(Maze_Watershed,C2);title('Shrinked Path')
%So now we can easily Take difference of both images to get the path.
P1 = Maze_Watershed == 2;
P2 = C2 == 2;
path = P1 - P2;
Solved = imoverlay(Maze_Binary, path, [0.25 0.25 1]); 
figure,imshow(Solved);title('Solved Maze') 