clc
close all

Filter  = fir1(4,0.6);

fprintf('%.8ff, %.8ff, %.8ff, %.8ff, %.8ff\n', Filter(1),Filter(2),Filter(3),Filter(4),Filter(5))