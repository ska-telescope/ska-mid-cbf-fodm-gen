close all

polyn = [0.0000000000000814823003138800,-0.0000000000016431430389022819, 0.0000000010907393954074044000,-0.0000767407379965083060000000,-1.2167180791444974000000000000,28863.4822125655080000000000000000];
polyn = [-0.0000000000000210335917845092, 0.0000000000003231712181835596, 0.0000000010797479298358872000,-0.0000766921336859014640000000,-1.2190195718046037000000000000,28845.2141783614720000000000000000];
polyn = [ 0.0000000000000063189099151256,-0.0000000000004269117185041496, 0.0000000010878372521577463000,-0.0000766678419509874480000000,-1.2201697713208526000000000000,28836.0672180964320000000000000000];
polyn = [0.0000000000000016765247099798,-0.0000000000000777454450817839, 0.0000000000013703522293644218,-0.0000000010937608165606331000, 0.0000767171471221868880000000, 1.2203389596567424000000000000,-28854.6047841830330000000000000000];
T = 8/1000;
n = 1000;
t_start = 0;
t_inct = T/n;

x = t_start:t_inct:t_start+T;

y = polyval(polyn, x);

pfit = polyfit(x, y, 1)

X = [ones(1, n+1) ; x]';

%X = single(X);
%y = single(y);

tao = inv(X'*X)*(X'*y')
invXX = inv(X'*X);
Xy = (X'*y');
tao1 = invXX(1,1)*Xy(1) + invXX(1,2)*Xy(2);
tao2 = invXX(2,1)*Xy(1) + invXX(2,2)*Xy(2);
y2 = polyval(tao', x);
diff_y = polyval([tao(2) tao(1)], x) - y;
% plot(diff_y)
% figure
% plot(diff_y./y)

lo_poly = table2array(readtable('FOTCP_SRC1_Receptor1_polX.csv'));
number_of_points = 800;
T_inct = (lo_poly(1,2) - lo_poly(1,1))/number_of_points;
y_lo_poly = zeros(1, size(lo_poly,1)*number_of_points);
size(lo_poly,1)
for i=1:size(lo_poly,1)
    t_start = lo_poly(i,1);
    t_stop = lo_poly(i,2);
    t_lo_poly = linspace(t_start, t_stop - T_inct,number_of_points) - t_start;
    y_lo_poly((i-1)*number_of_points+1:i*number_of_points) = polyval([lo_poly(i,3),lo_poly(i,4)], t_lo_poly);
end
t_ho_poly = linspace(lo_poly(1,1), lo_poly(end,2) - T_inct, size(lo_poly,1)*number_of_points) - lo_poly(1,1);
y_ho_poly = polyval(polyn, t_ho_poly);
plot(t_ho_poly+lo_poly(1,1), (y_lo_poly-y_ho_poly))
sum((y_lo_poly-y_ho_poly))
max(y_lo_poly-y_ho_poly)
xlabel('time stamp (s)') 
ylabel('Relative Error (%)') 
grid on
figure
hold on
plot(t_ho_poly, y_ho_poly, 'b-x')
plot(t_ho_poly, y_lo_poly, 'r-x')
grid on