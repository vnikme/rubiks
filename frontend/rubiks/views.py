# coding: utf-8


from django.http import HttpResponse
from django.template.loader import get_template
from django.utils.six.moves.urllib.parse import parse_qsl, urlparse, urlunparse
from django.utils.cache import patch_cache_control
from django.views.decorators.csrf import csrf_exempt
import httplib, time, base64, json


def do_general(request, body):
    t = get_template('general.html')
    html = t.render({'body': body})
    response = HttpResponse(html, content_type = 'text/html')
    patch_cache_control(response, max_age=0)
    return response


def debug_page(request):
    return do_general(request, 'debug.html')


def video_page(request):
    return do_general(request, 'video.html')


def main_page(request):
    return do_general(request, 'main.html')


def Solve(cube):
    try:
        if not cube:
            return None
        conn = httplib.HTTPConnection("localhost", 17071)
        conn.request("GET", "/solve?cube=%s" % cube, None)
        res = conn.getresponse()
        if res.status != 200:
            return None
        return res.read()
    except:
        return None


def solve_page(request):
    cube = request.GET.get("cube", "")
    answer = Solve(cube)
    if not answer:
        answer = """{"state": "fail", "message": "could not reach backend or bad parameters"}"""
    return HttpResponse(answer, content_type = "text/json")


def Log(data):
    try:
        if not data:
            return None
        conn = httplib.HTTPConnection("localhost", 17071)
        ts = time.time()
        data = json.loads(base64.b64decode(data))
        data["server_timestamp"] = ts
        data = base64.b64encode(json.dumps(data))
        conn.request("GET", "/log?data=%s" % data, None)
        res = conn.getresponse()
        if res.status != 200:
            return None
        return res.read()
    except:
        return None


def log_page(request):
    data = request.GET.get("data", "")
    answer = Log(data)
    if not answer:
        answer = """{"state": "fail", "message": "could not reach backend or bad parameters"}"""
    return HttpResponse(answer, content_type = "text/json")

