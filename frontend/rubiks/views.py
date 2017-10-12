# coding: utf-8


from django.http import HttpResponse
from django.template.loader import get_template
from django.utils.six.moves.urllib.parse import parse_qsl, urlparse, urlunparse
from django.utils.cache import patch_cache_control
from django.views.decorators.csrf import csrf_exempt

def do_general(request, body):
    t = get_template('general.html')
    html = t.render({'body': body})
    response = HttpResponse(html, content_type = 'text/html')
    patch_cache_control(response, max_age=0)
    return response

def main_page(request):
    return do_general(request, 'main.html')

