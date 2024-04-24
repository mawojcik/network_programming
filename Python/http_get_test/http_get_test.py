import sys
import requests

def print_server_data(website_url):
    ports_numbers = [80, 443]
    for port_number in ports_numbers:
        response = requests.get(f'{website_url}:{port_number}')
        data = response.headers.get('Server')
        print(f'port {port_number}: {data}')


if __name__ == '__main__':
    print_server_data(sys.argv[1])
