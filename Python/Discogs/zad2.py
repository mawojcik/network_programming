import sys
import requests
import json


class Artist:
    def __init__(self, name, bands):
        self.name = name
        self.bands = bands


class Band:
    def __init__(self, name, band_members=None):
        self.name = name
        if band_members is None:
            band_members = []
        self.band_members = band_members

    def __eq__(self, other):
        if isinstance(other, Band):
            return self.name == other.name
        return False

    def __gt__(self, other):
        return self.name > other.name

    def add_member(self, new_member):
        if new_member not in self.band_members:
            self.band_members.append(new_member)


def request_artist_data(website_url):
    try:
        response = requests.get(website_url)
        if response.status_code == 200 and 'application/json' in response.headers.get('content-type'):
            data = json.loads(response.text)
            if 'groups' not in data:
                print(f"No artist with ID: {data['id']} found!")
                sys.exit(1)
            groups = []
            for group in data['groups']:
                groups.append(group['name'])
            return Artist(data['name'], groups)
        elif response.status_code == 429:
            print("Too many requests!")
            sys.exit(1)
    except requests.exceptions.RequestException:
        print("Problem with sending API request")
    sys.exit(1)


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Provide at least 2 authors IDs")
        sys.exit(1)

    artistsList = []
    for arg in sys.argv[1:]:
        if arg.isdigit():
            artistsList.append(request_artist_data(f'https://api.discogs.com/artists/{arg}'))
        else:
            print("Input should be only numbers!")
            sys.exit(1)

    bandsList = []
    for artist in artistsList:
        for band_name in artist.bands:
            new_band = Band(band_name)
            if new_band not in bandsList:
                bandsList.append(new_band)
            for band in bandsList:
                if band.name == band_name:
                    band.add_member(artist.name)

    bandsList.sort()
    for band in bandsList:
        if len(band.band_members) > 1:
            print(f"Band: {band.name}, Members: {', '.join(band.band_members)}")
