import sys
import requests
import json

def validate_website(website_url):
    try:
        response = requests.get(website_url)
        # if response.status_code == 200 and 'text/html' in response.headers.get('content-type'):
        if response.status_code == 200:
            # data = response.text
            data = json.loads(response.text)

            releases = data['releases']
            for release in releases:
                # print(f"ID: {release['id']}")
                print(f"Title: {release['title']}")
                # print(f"Type: {release['type']}")
                # print(f"Main Release: {release.get('main_release', 'N/A')}")
                # print(f"Artist: {release['artist']}")
                # print(f"Role: {release['role']}")
                # print(f"Resource URL: {release['resource_url']}")
                # print(f"Year: {release['year']}")
                # print(f"Thumbnail URL: {release['thumb']}")
                # print(
                #     f"Community Stats - Wantlist: {release['stats']['community']['in_wantlist']}, Collection: {release['stats']['community']['in_collection']}")
                # print(
                #     f"User Stats - Wantlist: {release['stats']['user']['in_wantlist']}, Collection: {release['stats']['user']['in_collection']}")
                # print()


    except requests.exceptions.RequestException:
        pass
    sys.exit(1)


if __name__ == '__main__':
    validate_website('https://api.discogs.com/artists/516820/')
    # validate_website('https://api.discogs.com/artists/359282/releases?token=RjpRegXvJLpSPqLJJUkuRLCwgcYqvgpGgmLCtPgQ')

# ADD TOKEN