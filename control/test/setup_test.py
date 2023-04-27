import requests
import json

api_url = "http://localhost:8888/api/0.1/ngpd/"

def get(api_url):

    response = requests.get(api_url)
    json_response = json.loads(response.text)

    return json_response

def put(api_url, data):
    response = requests.put(api_url, json=data)
    return json.loads(response.text)

if __name__ == "__main__":
    print("ooo test")
    print(api_url)

    # print(get(api_url))

    print("\nBEGIN SETUP\n")
    put(api_url + "setup", {"setup_adq": True})
    put(api_url + "filter", {"setup_filter": True})
    put(api_url + "trigger", {"setup_trigger": True})
    put(api_url + "base_sub", {"setup_base_sub": True})
    put(api_url + "measure", {"setup_measure": True})
    put(api_url + "scope_options", {"setup_scope_options": True})
    put(api_url + "scope_options", {"setup_scope_streams": True})
    put(api_url + "scope_options", {"start_scope": True})
