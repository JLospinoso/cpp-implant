#
# This file is subject to the terms and conditions defined in
# file 'LICENSE', which is part of this source code package.
#

import http.server
import json
import os
import uuid
import datetime


class TaskManager:
    def __init__(self, tasking_path):
        self._tasking_path = tasking_path

    def handle(self, results, client, dtg):
        tasks = []
        if os.path.exists(self._tasking_path):
            try:
                with open(self._tasking_path, "r") as tasking:
                    tasks = json.load(tasking)
                    for task in (x for x in tasks if "id" not in x):
                        task["id"] = str(uuid.uuid4())
            except Exception as e:
                print("Error parsing tasking {}: {}".format(self._tasking_path, e))
        result_str = json.dumps({
            "datetime": dtg,
            "client": client,
            "new-tasking": tasks,
            "results": results
        }, sort_keys=True, indent=4)
        with open("result-{}.json".format(uuid.uuid4()), "w") as out_file:
            out_file.write(result_str)
        return json.dumps(tasks)


class CncHandler(http.server.BaseHTTPRequestHandler):
    _task_manager = TaskManager("tasks.json")

    def do_POST(self):
        content_len_header = self.headers.get('Content-Length')
        if content_len_header:
            try:
                content_len = int(content_len_header)
                post_body = self.rfile.read(content_len)
                results = json.loads(post_body)
                response = self._task_manager.handle(results, self.address_string(), self.date_time_string())
                self.send_response(202)
                self.wfile.write(response.encode("ascii"))
                return
            except Exception as e:
                print("Exception handling POST: {}".format(e))
        self.send_response(401)


if __name__ == "__main__":
    server_address = ('', 8000)
    httpd = http.server.ThreadingHTTPServer(server_address, CncHandler)
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
