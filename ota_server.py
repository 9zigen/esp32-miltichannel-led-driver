import http.server
import socketserver
import os

PORT = 8070

web_dir = os.path.join(os.path.dirname(__file__), 'build')
os.chdir(web_dir)

Handler = http.server.SimpleHTTPRequestHandler
httpd = socketserver.TCPServer(("", PORT), Handler)
print("serving at port", PORT)
httpd.serve_forever()