from flask import Flask, request, redirect, url_for, render_template
import requests as req
import json
import threading
import time

flag = 0
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
	while 1:
		r = ""
		devolucion = req.get ( "http://192.168.100.128/lec?todo" )
		if type (devolucion.text) is str:
			try: 
				decode = json.loads ( devolucion.text )
			except:
				pass
		time.sleep (3)

def cmd ( peticion ):
	print ( decode )
	print ( peticion )

def main ():
    lectura = threading.Thread ( target = lec )
    lectura.start ()
    app.run( host="0.0.0.0")


app = Flask(__name__)

@app.route ( "/", methods = [ "GET", "POST" ] )
def home():
    return render_template ( "monitor.html", dict = decode )  

@app.route ( "/configTemp", methods = [ "GET", "POST" ] ) 
def temp():
	if "tempMaxF" in request.form: 
		cmd ( ( "tempMaxF", request.form ["tempMaxF"] ) )
	elif "tempMinF" in request.form: 
		cmd ( ( "tempMinF", request.form ["tempMinF"] )  )

	return render_template ( "configTemp.html", dict = decode ) 

@app.route ( "/configTomacorrientes", methods = [ "GET", "POST" ] )  
def htomas(): 
	print ( request.form ) 
	for i in range (1,6):
		if f"{i}" in request.form: 
			cmd ( request.form [f"{i}"], f"tomas{i}" )
	return render_template ( "configTomas.html", dict = decode )  

@app.route ( "/configIP", methods = [ "GET", "POST" ] ) 
def ip():
    if "dhcp" in request.form: 
        cmd ( ( "dhcp", request.form ["dhcp"] ) )
    elif "conf" in request.form: 
        cmd ( ( "conf", request.form ["conf"] )  )
    elif "cancel" in request.form: 
        cmd ( ( "cancel", request.form ["cancel"] ) )
    elif "formIp" in request.form: 
        cmd ( ( "formIp", request.form ["formIp"]) )
    elif "reiniciar" in request.form: 
        cmd ( ( "reiniciar", request.form ["reiniciar"] ) )
        
    return render_template ( "ip.html", ipArd = "192.168.100.150", dict = decode )  


if __name__ == "__main__":
    main ()
