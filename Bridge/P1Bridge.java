public class P1Bridge {
  static {
    System.loadLibrary("p1bridge"); // Load native library p1bridge.dll (Windows) or libp1bridge.so (Unixes)
  }

  public native double compute(int numthreads, int modo, int longArray, int op);
}
