import sys
import requests


def validate_website(website_url):
    try:
        response = requests.get(website_url)
        if response.status_code == 200 and 'text/html' in response.headers.get('content-type'):
            data = response.text
            if 'Institute of Theoretical Physics' in data:
                print('valid website')
                sys.exit(0)
    except requests.exceptions.RequestException:
        print('invalid website')
    sys.exit(1)


if __name__ == '__main__':
    validate_website('http://th.if.uj.edu.pl/')
