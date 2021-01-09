ifeq ($(GPP),)
GPP=g++
endif

SRC=$(wildcard *.cpp)
TAR=$(basename $(SRC))

%:%.cpp
	$(GPP) -o $@ -g $< -I ../.. -std=c++11 ../thirdparty/libbson/lib/libbson-1.0.a  -pthread
	@echo ============ run $@ ================
	@valgrind --tool=memcheck --leak-check=full ./$@
	@-rm $@

tar:$(TAR)
	@echo -------test done-------
