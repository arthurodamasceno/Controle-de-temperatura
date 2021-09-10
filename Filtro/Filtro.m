clc
close all

Filter  = fir1(32,0.25);
freqz(Filter,1,128)

%fprintf('%.8ff, %.8ff, %.8ff, %.8ff, %.8ff\n', Filter(1),Filter(2),Filter(3),Filter(4),Filter(5))
X = [25 linspace(25,25,500) linspace(25,0,100) linspace(0,0,400)]
N = 0.3*randn(1001, 1);
S = X+N'
Y = filter(Filter,1,S)
figure(2)
plot(Y)
hold on
plot(S)