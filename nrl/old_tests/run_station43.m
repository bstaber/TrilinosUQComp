clc
clearvars
close all

% [ln,lt,delta] = meshgrid(1e-2*(5:5:20)*sqrt(50^2+25^2), ...
%                          1e-2*(4:4:16)*sqrt(50^2+25^2), ...
%                          0.1:0.1:0.2);
%
% [ln2,lt2,delta2] = meshgrid(1e-2*(5:5:25)*sqrt(50^2+25^2), ...
% 1e-2*(4:4:20)*sqrt(50^2+25^2), ...
% 0.1:0.1:0.2);

%ln = ln(:); lt = lt(:); delta = delta(:);

%modelParameters.mu      = 1e3*[1.7212, 0.0426, 0.0429, 1.3138, 0.0609];
%modelParameters.beta    = [27.9525, 0.0306]; %error in beta5 ...

% modelParameters.mu   = 1e3*[1.706486189075632, 0.057388351625262, 0.059077990032629, ...
%                             1.314292350043600, 0.061332096908722, 27.971878357799234, 0.039848773492742];
% modelParameters.beta = [27.971878357799234, 0.039848773492742];

modelParameters.mu   = 1e3*[1.7710, 0.0658, 0.0680, 1.4152, 0.0718];
modelParameters.beta = [25.4185, 0.0432];

optimParameters.station = 43;
optimParameters.np      = 28;

load('eij.mat');

angle_to_id = [5,6; 1,4; 2,3; 7,8];
Yexpi = cell(4,1);
for j = 1:4
    ID = angle_to_id(j,:);
    for k = 1:2
        Exx = [exx{ID(k)}{1}, exx{ID(k)}{2}];
        Eyy = [eyy{ID(k)}{1}, eyy{ID(k)}{2}];
        Exy = [exy{ID(k)}{1}, exy{ID(k)}{2}];
        Yexpi{j}(:,k) = log(sum(Exx.^2 + Eyy.^2 + 2*Exy.^2,1));
    end
end

fd = fopen(strcat('/home/s/staber/Trilinos_results/nrl/random_generator_for_pca_likelihood/station',num2str(optimParameters.station),'/output.txt'),'w');


I = [3 5; 4 5; 5 5; 4 4];
ln = 1e-2*(5:5:25)*sqrt(50^2+25^2);
lt = 1e-2*(4:4:20)*sqrt(50^2+25^2);
output = cell(length(ln),1);

for l = 1:5
 modelParameters.lc    = [ln(I(l,1)), lt(I(l,2))];
 modelParameters.delta = repmat(0.20,1,4);

 optimParameters.tol   = 1e-6;
 optimParameters.nmc   = 100;

 output{l} = costFunction(modelParameters,optimParameters,Yexpi);
 fprintf(fd,'%d \t %f \t %f \t %f \t %f\n',l,modelParameters.lc(1),modelParameters.lc(2),modelParameters.delta(1),output{l}.fval);
 save(strcat('result_station',num2str(optimParameters.station),'_nmc=100_ShinozukaCorrected_delta20.mat'),'output','-v7.3');
end