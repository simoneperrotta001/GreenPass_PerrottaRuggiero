cmake_minimum_required(VERSION 3.24)
project(GreenPass_PerrottaRuggiero C)

set(CMAKE_C_STANDARD 11)

include_directories(src)

add_executable(GreenPass_PerrottaRuggiero
        src/centroVaccinale.c
        src/centroVaccinale.h
        src/clientUtente.c
        src/clientUtente.h
        src/clientS.c
        src/clientS.h
        src/clientT.c
        src/clientT.h
        src/GreenPassUtility.c
        src/GreenPassUtility.h
        src/WrapperFunction.c
        src/WrapperFunction.h
        src/serverG.c
        src/serverG.h
        src/serverV.c
        src/serverV.h
        src/UsageUtility.c
        src/UsageUtility.h)
