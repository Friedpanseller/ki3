<html>
	<head>
		<script src="https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js"></script>
		<script src="../buttonScript.js"></script>
        <!--<script src='https://www.google.com/recaptcha/api.js'></script>-->
		<link rel="stylesheet" type="text/css" href="../buttonCss.css" />
		<style>
			body {
				font-family: Helvetica;
				margin: 0;
				padding: 0;
				background-color: #0e83cd;
				color: white;
                text-align: center;
			}
			h1 {
				font-size: 80px;
			}
            h2 {
				font-size: 40px;
			}
            h3 {
                font-size: 60px;
            }
            span {
                font-size: 24px;
                color: black;
                font-weight: bold;
            }
            .success {
                color: lawngreen;
            }
            #logfilediv {
                display: none;
            }
		</style>
	</head>
	<body>
        <br />
        <h1>UPLOADING FILES</h1>
        <h3 id="logfile"></h3>
        <button id="logfilediv" onclick="gogogo();" class="btn btn-1 btn-1e">View Game</button>
	</body>
</html>

<?php
	ob_start();  
        
    $teamID = $_POST["teamID"];
    
    if($teamID == escapeshellcmd($teamID)) {
        $teamID = escapeshellcmd($teamID);
        //if they DID upload a file...
        if(file_exists($_FILES["AI_A"]["tmp_name"]) 
              && file_exists($_FILES["AI_B"]["tmp_name"]) 
              && file_exists($_FILES["AI_C"]["tmp_name"])) {

            echo "<script>$('#logfile').html('Please wait');</script>";

            echo "<h2>";

            echo str_repeat("<!-- AgentDisguise -->", 1000); 
            
            ob_flush();
            flush();

            move_uploaded_file($_FILES["AI_A"]["tmp_name"],$teamID . "_AI_A.c");
            move_uploaded_file($_FILES["AI_B"]["tmp_name"],$teamID . "_AI_B.c");
            move_uploaded_file($_FILES["AI_C"]["tmp_name"],$teamID . "_AI_C.c");

            echo "Compiling AI_A: " . $_FILES["AI_A"]["name"] . "<br />";
            echo str_repeat("<!-- AgentDisguise -->", 1000);
            ob_flush();
            flush();

            exec("gcc -std=c99 -shared -fPIC -olibdlo.so -o ai_1.so '" . $teamID . "_AI_A.c' 2>&1", $err1);
            if(!empty($err1)) {
                echo "<span>";
                var_dump($err1);
                echo "<br /></span>";
            } else {
                echo "<span class='success'>Compile Success<br /></span>";
            }

            echo "Compiling AI_B: " . $_FILES["AI_B"]["name"] . "<br />";
            echo str_repeat("<!-- AgentDisguise -->", 1000);
            ob_flush();
            flush();
            exec("gcc -std=c99 -shared -fPIC -olibdlo.so -o ai_2.so '" . $teamID . "_AI_B.c' 2>&1", $err2);
            if(!empty($err2)) {
                echo "<span>";
                var_dump($err2);
                echo "<br /></span>";
            } else {
                echo "<span class='success'>Compile Success<br /></span>";
            }

            echo "Compiling AI_C: " . $_FILES["AI_C"]["name"] . "<br />";
            echo str_repeat("<!-- AgentDisguise -->", 1000);
            ob_flush();
            flush();
            exec("gcc -std=c99 -shared -fPIC -olibdlo.so -o ai_3.so '" . $teamID . "_AI_C.c' 2>&1", $err3);
            if(!empty($err3)) {
                echo "<span>";
                var_dump($err3);
                echo "<br /></span>";
            } else {
                echo "<span class='success'>Compile Success<br /></span>";
            }
            if(empty($err1) && empty($err2) && empty($err3)) {
                echo "Running Game<br />";
                echo str_repeat("<!-- AgentDisguise -->", 1000);
                ob_flush();
                flush();
                sleep(15);
                $counter = 0;
                while(file_exists("log_" . $teamID . $counter . ".txt")) {
                    $counter++;
                }
                exec("./Game '" . $teamID . $counter . "' > 'rLog_"  . $teamID . $counter . ".txt'");
                if(file_exists("rLog_" . $teamID . $counter . ".txt")) {
                    echo "<span>";
                    echo file_get_contents("rLog_" . $teamID . $counter . ".txt", false);
                    echo "</span>";
                } else {
                    echo "<span class='success'>Game Run Success<br /></span>";
                }

                if(file_exists("log_" . $teamID . $counter . ".txt")) {
                    echo "</h2><script>$('#logfile').html('Your Log file is: " . $teamID . $counter . "'); $('#logfilediv').fadeIn(300);</script>";
                } else {
                    echo "<h3>Please check compile errors</h3>";
                }

                echo "<script>function gogogo() { window.location.replace('../index.html#". $teamID . $counter . "')}</script>";
                
                echo "<br />Deleting AI_A";
                unlink($teamID . "_AI_A.c");
                echo "<br />Deleting AI_B";
                unlink($teamID . "_AI_B.c");
                echo "<br />Deleting AI_C";
                unlink($teamID . "_AI_C.c");
                
            } else {
                 echo "<h3>Please check compile errors</h3>";
            }
        } else {
            echo "<h3>Please upload 3 AI files</h3>";
        }
    } else {
        echo "<h3>Invalid characters in team name</h3>";
    }
?>