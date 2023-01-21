

public class P1 {
   public static void main(String[] args) {

     Config config = new Config();
     boolean DEBUG = false;
     int op = 0;
     int modo = 0;

     if(args[0].equalsIgnoreCase("--help")){
         System.out.println("\nModo de empleo: cli [OPTIONS] numDouble operacion\nOpera los n primeros numeros, desde 0 hasta (numDouble - 1)\nOperaciones admitidas: sum, sub, xor\n[Options]\n     --multi-thread  Realiza las operaciones de forma paralela con los threads\n                     que le introduzcas [Maximo threads = 12].\n                     Esta opcion no es obligatoria, en caso de no ponerla o\n                     ponerla igual a 0 el calculo lo realizara el main.\n\n");
         return;
     }

     if(args.length < 2){
         System.out.println("introduce los argumentos correctamente\n./cli [Options] numDoubles Operacion\n");
         return;
     }

     int posLong = 0;
     int posOp = 1;
     int i;
     for(i = 0; i < args.length; i++){
         if(args[i].equalsIgnoreCase("--multi-thread")){
             i++;
             if(isDigit(args[i])){
                 config.setNumthreads(Integer.parseInt(args[i]));
             } else {
                 System.out.println("Introduce numero de threads correctamente, --multi-thread numThreads\nPara mas informacion : cli --help\n");
                 return;
             }

             if(config.getNumthreads() > 12) {System.out.println("Excedido el numero de threads\n"); return;}
             if(i == 1){
                 posLong = 2; posOp = 3;
                 //if(DEBUG){
                 //System.out.println("PosLOng = %d, PosOp = %d\n", posLong, posOp);}
                 }
             if(i == 2){posOp = 3;}
         }else if(i == posLong ){
             if(isDigit(args[i])){
                 config.setLongArray(Integer.parseInt(args[i]));
             } else {
                 System.out.println("Cantidad de doubles no entontrada. Para mas informacion : cli --help\n");
                 return;
             }

         }else if (i == posOp) {

             if (args[i].equalsIgnoreCase("sum")){
                 config.setOp(operation.sum);
                 op = 0;
             }
             else if (args[i].equalsIgnoreCase("sub")){
                 config.setOp(operation.sub);
                 op = 1;
             }
             else if (args[i].equalsIgnoreCase("xor")){
                 config.setOp(operation.xor);
                 op = 2;
             }else{
                 System.out.println("Operacion desconocida");
                 return;
             }

         } else{
             System.out.println("Problema al introducir los parametros. Para mas informacion : cli --help");
         }
     }

    if(config.getNumthreads() == 0){
        config.setMode(mode.single);
    }else {
        config.setMode(mode.multi);
    }

     if(config.getMode() == mode.single){
         modo = 0;
     } else {
        modo = 1;
     }


     if(DEBUG) {
     System.out.println("\n------PARAMETROS DEL PROGRAMA-------\n");
     System.out.println("args.length = " + args.length);
     for(int j = 0; j < args.length; j++){
     System.out.println("\n Argumento" + j + " = " + args[j]);
     }
     System.out.println("\nlonguitud del array = " + config.getLongArray());
     System.out.println("\noperacion seleccionada (sum, sub, xor) = " + config.getOp());
     System.out.println("\nnumero de threads = " + config.getNumthreads());
     System.out.println("\nModo de ejecucion (single, multi) = " + config.getMode() + "\n\n\n");
     }

     //----------LLAMAMOS C++-------
    //config.getNumthreads(), config.getMode(), config.getLongArray(), config.getOp()
     double result = (new P1Bridge()).compute(config.getNumthreads(), modo, config.getLongArray(), op);



     System.out.println("Solucion = " + result + "\n");

     return;
   }

   public static boolean isDigit (String x) {
       try {
           Integer.parseInt(x);
           return true;
       }catch (NumberFormatException e) {
           return false;
    }
   }


}
