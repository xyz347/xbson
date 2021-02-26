ifeq ($(GPP),)
GPP=g++
endif

INC=-I ./.. -I../.. -I ../../googletest/googletest/include  -I /usr/include/qt4  -I /usr/include/qt4/QtCore
LIB=-L ../../googletest/lib -l gtest -pthread -lQtCore ../thirdparty/libbson/lib/libbson-1.0.a # for gtest and qt test

xtest:
	$(GPP) -o $@ -g test.cpp -std=c++11 -DXPACK_SUPPORT_CHAR_ARRAY -DXPACK_SUPPORT_QT $(INC) $(LIB) $(XFLAG) -Wall -Wextra
	@-valgrind --tool=memcheck --leak-check=full  ./$@
	#@- ./$@
	@-rm $@
