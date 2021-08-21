clc
close all

Filter  = fir1(4,0.2);

fprintf('%.8ff, %.8ff, %.8ff, %.8ff, %.8ff', Filter(1),Filter(2),Filter(3),Filter(4),Filter(5))