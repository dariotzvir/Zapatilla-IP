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

TEMPMAX = 125
TEMPMIN = -40

def ipValida ( ipStr ):
    flag = 1
    if type ( ipStr ) is str and ipStr is not None: 
        vec = ipStr.split ( "." )
        if len ( vec ) != 4:
            flag = 0
        else:
            for i in vec:
                try:
                    i = int ( i )
                    if i > 255 or i < 0:
                        flag = 0
                except:
                    flag = 0
    else: 
        flag = 0
    return flag
    
    #DE:AD:BE:EF:FE:ED
def macValida ( mac ):
    flag = 1
    if type ( mac ) is str and mac is not None:
        vec = mac.split ( ":" )
        if len ( vec ) != 6:
            flag = 0
        else:
            for i in vec:
                try: 
                    i = int ( i, 16 )
                    if i > 255 or i < 0:
                        flag = 0
                except:
                    flag = 0  
    else: 
        flag = 0
    return flag

def lec ():
    global decode
    while 1:
        devolucion = req.get ( (f"http://{ipArduino}/lec?todo") )
        if type (devolucion.text) is str:
            try: 
                decode = json.loads ( devolucion.text )
            except:
                pass
        time.sleep (4)
        #print (decode)

def cmd ( request ):
    global ipArduino
    print ( request )
    if "tempmax" in request:
        tM = int ( request ['tempmax'] )
        if tM > decode ['tempMin'] and tM <= TEMPMAX:
            req.get ( (f"http://{ipArduino}/cmd?tempmax={tM}") )
        
    elif "tempmin" in request:
        tm = int ( request ['tempmin'] )
        if tm > decode ['tempMax'] and tm >= TEMPMIN:
            req.get ( (f"http://{ipArduino}/cmd?tempmin={tm}") )
        
    elif "ipdef" in request and ipValida ( request ['ipdef'] ):
        req.get ( (f"http://{ipArduino}/cmd?tempmin={request ['ipdef']}"), timeout=1 )
        if decode ["dhcp"] == 0:
            ipArduino = request ['ipDef']
            
    elif "dhcp" in request:
        aux = int ()
        if request ["dhcp"] == 'On': aux = 1
        else: aux = 0
        req.get ( (f"http://{ipArduino}/cmd?dhcp={aux}") )
        if aux == 0: ipArduino = decode ["ipDef"]
        
    elif "mac" in request and macValida ( request ["mac"] ):
        req.get ( (f"http://{ipArduino}/cmd?mac={request ['mac']}") )


def main ():
    #lec ()
    lectura = threading.Thread ( target=lec )
    comandos = threading.Thread ( target=cmd )
    lectura.start ()
    app.run( host="0.0.0.0", debug = 1 )


app = Flask(__name__)

@app.route ( "/", methods = ["GET", "POST"] )
def home ():
    return render_template ( "monitor.html", dict=decode )  

@app.route ( "/configTemp", methods=["GET", "POST"] ) 
def temp ():
    if len ( request.form ):
        cmd ( request.form )

    return render_template ( "configTemp.html", dict=decode ) 

@app.route ( "/configTomacorrientes", methods=["GET", "POST"] )  
def htomas (): 
    if len ( request.form ):
        cmd ( request.form )

    return render_template ( "configTomas.html", dict=decode )  

@app.route ( "/configIP", methods = ["GET", "POST"] ) 
def ip ():
    if len ( request.form ):
        cmd ( request.form )
        
    return render_template ( "ip.html", ipArd=ipArduino, dict=decode )  


if __name__=="__main__":
    main ()
