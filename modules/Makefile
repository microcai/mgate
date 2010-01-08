
DEBUG := -g 

CXXFLAGS := ${DEBUG}  -fPIC -I../libmicrocai
LDFLAGS := -L${MONITOR_LIB} -lmicrocai -lksql -lpthread -L.

MONITOR_LIB:=../monitor/lib/

TARGET = kqq.so kmsn.so kurl.so kpolice.so kroom.so klz.so
BUILD_SERVER :=
# kserver.so

all:libksql.so $(TARGET) $(BUILD_SERVER) Makefile

libksql.so:kmysql.cpp kmysql.h
	@echo -n Compiling $@ for other modules ...
	@$(CXX) $< -o $@  -L${MONITOR_LIB} -lmicrocai  -L/usr/lib64/mysql -lmysqlclient $(CXXFLAGS) -shared
	@cp $@ ${MONITOR_LIB}
	@echo Done!
	

kserver.so:kServer.cpp kServer.hpp kmysql.h
	@$(CXX) -shared $< -o $@	 $(LDFLAGS) $(CXXFLAGS)
	@echo Build $(BUILD_SERVER) 

clean:
	@echo -n cleanning ...
	@$(RM) *.so
	@echo Done.
#	同志们，Makefile 的规则就是这么伟大啊！哈哈。
%.so: %.cpp kmysql.h
	@echo -n Compiling $@ with $(DEBUG)....
	@$(CXX)  $< -o $@ $(LDFLAGS) $(CXXFLAGS) -shared
	@mkdir -p ${MONITOR_LIB}
	@echo Done!
	@cp $@ ${MONITOR_LIB}
	
