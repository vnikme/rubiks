# coding: utf-8


from django.http import HttpResponse
from django.template.loader import get_template
from django.utils.six.moves.urllib.parse import parse_qsl, urlparse, urlunparse
from django.utils.cache import patch_cache_control
from django.views.decorators.csrf import csrf_exempt
import urllib.request, time, base64, json


def do_general(request, body):
    t = get_template('general.html')
    html = t.render({'body': body})
    response = HttpResponse(html, content_type='text/html')
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
        request = urllib.request.Request("http://localhost:17071/solve?cube={}".format(cube), method='GET')
        response = urllib.request.urlopen(request)
        if response.status != 200:
            return None
        return response.read()
    except Exception as e:
        return None


def solve_page(request):
    cube = request.GET.get("cube", "")
    answer = Solve(cube)
    if not answer:
        answer = """{"state": "fail", "message": "could not reach backend or bad parameters"}"""
    return HttpResponse(answer, content_type="text/json")


def Log(data):
    try:
        if not data:
            return None
        data = json.loads(base64.b64decode(data))
        data["server_timestamp"] = time.time()
        data = base64.b64encode(json.dumps(data))
        request = urllib.request.Request("http://localhost:17071/log".format(cube), data=data)
        response = urllib.request.urlopen(request)
        if response.status != 200:
            return None
        return ressponse.read()
    except Exception as e:
        return None


@csrf_exempt
def log_page(request):
    data = request.body
    answer = Log(data)
    if not answer:
        answer = """{"state": "fail", "message": "could not reach backend or bad parameters"}"""
    return HttpResponse(answer, content_type="text/json")

