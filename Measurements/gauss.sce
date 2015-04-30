// ------ init -----
clc;
clear;
xdel(winsid())
// ------ init-end ----


//measuredValues= [122; 125; 123; 121; 119; 119; 117; 124; 120; 116];
measuredValues= [70200; 71000; 69200; 69000; 68700; 71400; 70400];

function f = gauss_distribution(x, mu, s)
p1 = -.5 * ((x - mu)/s) .^ 2;
p2 = (s * sqrt(2*%pi));
f = exp(p1) ./ p2;
endfunction

N=size(measuredValues,1);
arithMean = sum(measuredValues)/N;
mu = arithMean;
stdDev=sqrt(1/(N-1)*sum((measuredValues-arithMean).^2));
s = stdDev;

x=linspace(mu-3*s, mu+3*s, 1000);	//start, end, number of values


f = gauss_distribution(x, mu, s);
plot2d(x,f, style=2);	// change style-nr to change color, you can add several plot2d's too
plot2d(x,2*f, style=5);	// change style-nr to change color, you can add several plot2d's too
//grid on;
title('Gaussian distribution');
xlabel('measured values');
ylabel('Gauss Distribution');


