clc
close all
s = tf('s');
z = tf('z');

%ensaio25.sample = ensaio25.sample/2; %converter sample para segeundos (2Hz)


Max = max(ensaio25.temp);
Min = min(ensaio25.temp);
K = Max-Min

tau = (find(ensaio25.temp==ceil(K*0.632+Min),1)-160)/2

G = (K)/((tau*s)+1)


Ts = 1;
Gd = c2d(G,Ts,'zoh')

%% Projeto
wc = 2*pi*0.001
PMd = 80*pi/180

Gp_aux = Gd
[zeros,polos,k] = zpkdata(Gd)

PM = pi + angle(evalfr(Gp_aux,exp(1i*Ts*wc)))

theta = PMd - PM

theta_DEG = theta*180/pi

wzd = ( sin(Ts*wc)+2*sin(Ts*wc/2)^2 *tan(theta) ) / ( sin(Ts*wc)-2*sin(Ts*wc/2)^2 *tan(theta))

PIauxd = (z-wzd)/(z-1)

kPId = sign(k)/(abs(evalfr(Gp_aux,exp(1i*Ts*wc))) * abs(evalfr(PIauxd,exp(1i*Ts*wc))))

PI = kPId*(z-wzd)/(z-1);

zpk(PI)

%% analise controle

eq_car = 1+(Gd*PI)

malha_fechada = (Gd*PI)/eq_car
erro = 1 /eq_car
disturbio_carga = Gd /eq_car
acao_controle  = PI /eq_car

%% Plots
figure(1)
plot(ensaio25.sample(80:end)-80,ensaio25.temp(80:end),'r')
hold on
[amp,time] = step(G+Min);
plot(time,amp,'b','LineWidth',1.5)
grid on

figure(2)
step(G,Gd)

figure(3)
step(malha_fechada,erro)
grid on
figure(4)
step(disturbio_carga)
grid on
figure(5)
step(acao_controle)
grid on

figure(6)
margin(Gd*PI)
grid on

figure(7)
step(malha_fechada,Gd/87)
grid on

%% eq diferanças
[num,den] = tfdata(zpk(PI),'v')
[r,p,k] = residuez(num,den)

