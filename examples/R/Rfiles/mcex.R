# Code borrowed from Robert & Casella
#   Introducing Monte Carlo Methods with R
#   ISBN 978-1441915757
h=function(x){
  (cos(50*x)+sin(20*x))^2
}

par(mar=c(2,2,2,1),mfrow=c(2,1))
curve(h,xlab="Function",ylab="",lwd=2)
res <- integrate(h,0,1)
res

NSIM <- 10^4
x=h(runif(NSIM))
estint=cumsum(x)/(1:NSIM)
esterr=sqrt(cumsum((x-estint)^2))/(1:NSIM)

# Make the plot
outfile <- "integ.pdf"
pdf(file=outfile)
plot(estint, xlab="Mean and error range",type="l",lwd=
+ 2,ylim=mean(x)+20*c(-esterr[NSIM],esterr[NSIM]),ylab="")
lines(estint+2*esterr,col="gold",lwd=2)
lines(estint-2*esterr,col="gold",lwd=2)
dev.off()
