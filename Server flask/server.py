from flask import Flask, request, redirect, url_for, render_template
import requests as req
import json
import threading
import time

ipArduino = "192.168.100.128"

decode = {
	'tempMax': 0, 
	'tempMin': 0, 
	'tempAct': 0, 
	'humAct': 0, 
	'tension': 0, 
	'dhcp': 0, 
	'mac': '00 00 00 00 00 00', 
	'ipDef': '0.0.0.0', 
	'estTomas': [0, 0, 0, 0, 0], 
	'corriente': [0, 0, 0, 0, 0]
}


def lec ():
	global decode
	global ip
	while 1:
		r = ""
		devolucion = req.get ( (f"http://{ipArduino}/lec?todo") )
		if type (devolucion.text) is str:
			try: 
				decode = json.loads ( devolucion.text )
			except:
				pass
		time.sleep (3)

def cmd ( comando, valor ):
	pass

def main ():
    lectura = threading.Thread ( target=lec )
    lectura.start ()
    app.run( host="0.0.0.0", debug = 1 )


app = Flask(__name__)

@app.route ( "/", methods = ["GET", "POST"] )
def home ():
    return render_template ( "monitor.html", dict=decode )  

@app.route ( "/configTemp", methods=["GET", "POST"] ) 
def temp ():

	return render_template ( "configTemp.html", dict=decode ) 

@app.route ( "/configTomacorrientes", methods=["GET", "POST"] )  
def htomas (): 
	if request.form is not None:
		print ( request.form )

	return render_template ( "configTomas.html", dict=decode )  

@app.route ( "/configIP", methods = ["GET", "POST"] ) 
def ip ():
	if request.form is not None:
		cmd ( request.form )
        
    return render_template ( "ip.html", ipArd=ipArduino, dict=decode )  


if __name__=="__main__":
    main ()
