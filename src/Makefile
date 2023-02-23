all: clean clientUtente clientS clientT centroVaccinale serverV serverG
	
clientUtente: clientUtente.o GreenPassUtility.o WrapperFunction.o UsageUtility.o
	gcc clientUtente.o GreenPassUtility.o WrapperFunction.o UsageUtility.o -o clientUtente
	
clientUtente.o: clientUtente.c clientUtente.h GreenPassUtility.h WrapperFunction.h UsageUtility.h
	gcc -c clientUtente.c
	
	
clientS: clientS.o GreenPassUtility.o WrapperFunction.o UsageUtility.o
	gcc clientS.o GreenPassUtility.o WrapperFunction.o UsageUtility.o -o clientS
	
clientS.o: clientS.c clientS.h GreenPassUtility.h WrapperFunction.h UsageUtility.h
	gcc -c clientS.c
	
	
clientT: clientT.o GreenPassUtility.o WrapperFunction.o UsageUtility.o
	gcc clientT.o GreenPassUtility.o WrapperFunction.o UsageUtility.o -o clientT
	
clientT.o: clientT.c clientT.h GreenPassUtility.h WrapperFunction.h UsageUtility.h
	gcc -c clientT.c
	
	
centroVaccinale: centroVaccinale.o GreenPassUtility.o WrapperFunction.o UsageUtility.o
	gcc centroVaccinale.o GreenPassUtility.o WrapperFunction.o UsageUtility.o  -o centroVaccinale

centroVaccinale.o: centroVaccinale.c centroVaccinale.h GreenPassUtility.h WrapperFunction.h UsageUtility.h
	gcc -c centroVaccinale.c


serverV: serverV.o GreenPassUtility.o WrapperFunction.o UsageUtility.o
	gcc serverV.o GreenPassUtility.o WrapperFunction.o UsageUtility.o  -o serverV -pthread

serverV.o: serverV.c serverV.h GreenPassUtility.h WrapperFunction.h UsageUtility.h
	gcc -c serverV.c -pthread
	
	
serverG: serverG.o GreenPassUtility.o WrapperFunction.o UsageUtility.o
	gcc serverG.o GreenPassUtility.o WrapperFunction.o UsageUtility.o  -o serverG

serverG.o: serverG.c serverG.h GreenPassUtility.h WrapperFunction.h UsageUtility.h
	gcc -c serverG.c
	
	

UsageUtility.o: UsageUtility.c UsageUtility.h
	gcc -c UsageUtility.c -pthread
	
WrapperFunction.o: WrapperFunction.c UsageUtility.c WrapperFunction.h UsageUtility.h
	gcc -c WrapperFunction.c UsageUtility.c -pthread
	
GreenPassUtility.o: GreenPassUtility.c WrapperFunction.c UsageUtility.c GreenPassUtility.h WrapperFunction.h UsageUtility.h
	gcc -c GreenPassUtility.c WrapperFunction.c UsageUtility.c -pthread
	
clean:
	rm -f *.o all
	rm -f serverV
	rm -f clientT
	rm -f clientS
	rm -f clientUtente
	rm -f centroVaccinale
	rm -f serverG
