rebuild: build-mock-jni build-java build-cpp

# Step 1) create P1Bridge.java, get headers/JNI mock

build-mock-jni:
	javac -h . P1Bridge.java Enum.java

# Step 2) compile Java code

build-java:
	javac P1Bridge.java Enum.java; javac P1.java Config.java Enum.java;

# Step 3) create P1Bridge.cpp (linked to JNI/jni.h), get dynamic library

build-cpp:
	g++ -std=c++11 -fPIC -I"$$JAVA_HOME/include" -I"$$JAVA_HOME/include/linux" -shared -o libp1bridge.so P1Bridge.cpp

# 3) Run Java code

run:
	@/usr/bin/time java -Djava.library.path=. P1 --multi-thread 3 20 sum

clean:
	rm P1.class P1Bridge.class libp1bridge.so
