<?php
	//exec("ls");
	/*echo "Compiling AI_1.c...\r\n";
	shell_exec("sudo gcc -std=c99 -shared -fPIC -olibdlo.so -o ai_1.so AI_A.c");
	echo "Compiling AI_2.c...\r\n";
	shell_exec("sudo gcc -std=c99 -shared -fPIC -olibdlo.so -o ai_2.so AI_A.c");
	echo "Compiling AI_3.c...\r\n";
	shell_exec("sudo gcc -std=c99 -shared -fPIC -olibdlo.so -o ai_3.so AI_A.c");
	echo "Compiling Game.c...\r\n";
	shell_exec("sudo gcc -Wl,--no-as-needed -ldl -rdynamic -o Game Game.c");*/
	//echo "executing game...\r\n";
	//shell_exec("./Game");
    if($_POST["mode"] == "log") {
        if(file_exists("ai/log_" . $_POST["logName"] . ".txt")) {
            echo file_get_contents("ai/log_" . $_POST["logName"] . ".txt", false);
        } else {
            echo "404";
        }
    }
?>