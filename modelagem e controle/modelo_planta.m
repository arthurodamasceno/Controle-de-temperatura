clc
close all
s = tf('s');
z = tf('z');

%ensaio25.sample = ensaio25.sample/2; %converter sample para segeundos (2Hz)


Max = max(ensaio25.temp);
Min = min(ensaio25.temp);
K = Max-Min

tau = (find(ensaio25.temp==ceil(K*0.632+Min),1)-160)/2
taud = 20;

G = (K)/((tau*s)+1)
%G = (K*(exp(-taud*s)))/((tau*s)+1)

Ts = 1;
Gd = c2d(G,Ts,'zoh')
Gw = d2c(Gd,'tustin')

%% Projeto
wc = 2*pi*0.002
PMd = 85*pi/180

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
format longG
[r,p,k] = residuez(num,den)

%% outro controlador
PIs = tf([0.171531869083168      0.000788396688646132],[1 0])
dPIs= c2d(PIs,Ts,'zoh')

eq_car2 = 1+(Gd*dPIs)
acao_controle2  = dPIs /eq_car2
malha_fechada2 = (Gd*dPIs)/eq_car2
[num2,den2] = tfdata(dPIs,'v')
format longG
[r2,p2,k2] = residuez(num2,den2)

step(malha_fechada,Gd/87)
grid on
%% prints
ksat = 0.1;
fprintf('#define r %.8ff\n', r)
fprintf('#define p %.8ff\n', p)
fprintf('#define k %.8ff\n', k)
fprintf('#define ksat %.8ff\n\n', ksat)

fprintf('#define r %.8ff\n', r2)
fprintf('#define p %.8ff\n', p2)
fprintf('#define k %.8ff\n', k2)
fprintf('#define ksat %.8ff\n', ksat)