import json
import sys
import io
import time
sys.path.append('../gen-py')


from thrift import Thrift
from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol

from slidinghyper import SlidingHyperService


host, port = 'localhost', 9090
transport = TSocket.TSocket(host, port)
transport = TTransport.TBufferedTransport(transport)
protocol = TBinaryProtocol.TBinaryProtocol(transport)
client = SlidingHyperService.Client(protocol)
transport.open()

dump = io.open(sys.argv[1])

'''
dump format
{"v":2,"timestamp":1446731569,"values":["2d15b2e0"],"key":["c","100031","NumSimhashByImagehashes","c61d7fba70c089ff"]}
{"v":2,"timestamp":1446731569,"values":["hotmail.com"],"key":["c","100031","NumActorEmailDomainByIpCountry","GB"]}
{"v":2,"timestamp":1446731569,"values":["212.22.2.69"],"key":["c","100031","NumIpBySimhash","2d15b2e0"]}
'''
start_time = time.time()
last_time = time.time()

try:
    for i, line in enumerate(dump):
        data = json.loads(line)
        key, value = json.dumps(data['key']), json.dumps(data['values'])
        key = key.encode('base64')
        timestamp = data['timestamp']
        client.add_many(timestamp, key, [value])
        if i % 1000 == 0:
            l_time = time.time()
            print '%d processed. last 1000 took %.2f s total time %.2f s' % (i, l_time - last_time, l_time - start_time)
            last_time = l_time

    end_time = time.time()

finally:
    transport.close()
