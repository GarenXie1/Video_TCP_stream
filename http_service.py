import time
from http.server import BaseHTTPRequestHandler, HTTPServer

HOST_NAME = '192.168.8.127'
PORT_NUMBER = 8080

# 测试链接 ： http://192.168.8.127:8080/

class MyHandler(BaseHTTPRequestHandler):
    def do_HEAD(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        print(time.asctime(), "MyHandler -> do_HEAD ....")

    def do_GET(self):
        paths = {
            '/foo': {'status': 200},
            '/bar': {'status': 302},
            '/baz': {'status': 404},
            '/qux': {'status': 500}
        }
        print(time.asctime(), "MyHandler -> do_GET ....")
        if self.path in paths:
            self.respond(paths[self.path])
        else:
            self.respond({'status': 500})

    def handle_http(self, status_code, path):
        print(time.asctime(), "MyHandler -> handle_http ....")
        self.send_response(status_code)
        self.send_header('Content-type', 'text/html')
        self.end_headers()
        return bytes(b'I am HTTP server , garen')

    def respond(self, opts):
        print(time.asctime(),"MyHandler -> respond ....")
        response = self.handle_http(opts['status'], self.path)

        # 发送一个文件, StressTestLogFile.txt
        txt = open("StressTestLogFile.txt", "rb")
        while True:
            data = txt.read(1000)
            self.wfile.write(data)
            print(len(data))
            if len(data) == 0:
                break



if __name__ == '__main__':
    server_class = HTTPServer
    httpd = server_class((HOST_NAME, PORT_NUMBER), MyHandler)
    print(time.asctime(), 'Server Starts - http://%s:%s' % (HOST_NAME, PORT_NUMBER))
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    httpd.server_close()
    print(time.asctime(), 'Server Stops - http://%s:%s' % (HOST_NAME, PORT_NUMBER))
