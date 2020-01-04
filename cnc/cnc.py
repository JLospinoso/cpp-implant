#
# This file is subject to the terms and conditions defined in
# file 'LICENSE', which is part of this source code package.
#

import http.server


class CncHandler(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        message_parts = [
            'CLIENT VALUES:',
            'client_address=%s (%s)' % (self.client_address,
                                        self.address_string()),
            'command=%s' % self.command,
            'path=%s' % self.path,
            'request_version=%s' % self.request_version,
            '',
            'SERVER VALUES:',
            'server_version=%s' % self.server_version,
            'sys_version=%s' % self.sys_version,
            'protocol_version=%s' % self.protocol_version,
            '',
            'HEADERS RECEIVED:',
            ]
        for name, value in sorted(self.headers.items()):
            message_parts.append('%s=%s' % (name, value.rstrip()))
        message_parts.append('')
        message = '\r\n'.join(message_parts).encode('ascii')
        self.send_response(200)
        self.end_headers()
        self.wfile.write(message)
        return


if __name__ == "__main__":
    server_address = ('', 8000)
    httpd = http.server.HTTPServer(server_address, CncHandler)
    httpd.serve_forever()
