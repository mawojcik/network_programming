#!/usr/bin/python3
# -*- coding: UTF-8 -*-

'''
Aplikacja WSGI implementująca najważniejsze części opisywanej na wykładzie
usługi REST dającej dostęp do bazy z danymi osób.

Uwaga: kod dydaktyczny bez pełnej obsługi błędów i sytuacji nadzwyczajnych.

Aplikacja nie potrafi sama stworzyć swojej bazy danych, trzeba to zrobić
przed jej uruchomieniem. Skrypt rest_webapp.sh pokazuje jak.
'''
from urllib.parse import parse_qs

plik_bazy = './osoby.sqlite'

import re, sqlite3


class OsobyApp:
    def __init__(self, environment, start_response):
        '''
Konstruktor wywoływany przez serwer WSGI. Jak każdy konstruktor tworzy nowy
obiekt, następnie zapamiętuje w jego polach przekazane przez serwer argumenty
i inicjuje pola na odpowiedź.
'''
        self.env = environment
        self.start_response = start_response
        self.status = '200 OK'
        self.headers = [('Content-Type', 'text/html; charset=UTF-8')]
        self.content = b''

    def __iter__(self):
        '''
Metoda obsługująca proces iterowania po stworzonym obiekcie. Serwer WSGI
wymaga aby w środku była co najmniej jedna instrukcja "yield" zwracająca
ciąg bajtów do odesłania klientowi HTTP.
'''
        try:
            self.route()
        except sqlite3.Error as e:
            s = 'SQLite error: ' + str(e)
            self.failure('500 Internal Server Error', s)
        n = len(self.content)
        self.headers.append(('Content-Length', str(n)))
        self.start_response(self.status, self.headers)
        yield self.content

    def failure(self, status, detail=None):
        '''
Metoda wstawiająca do pól obiektu status błędu oraz dokument HTML
z komunikatem o jego wystąpieniu.
'''
        self.status = status
        s = '<html>\n<head>\n<title>' + status + '</title>\n</head>\n'
        s += '<body>\n<h1>' + status + '</h1>\n'
        if detail is not None:
            s += '<p>' + detail + '</p>\n'
        s += '</body>\n</html>\n'
        self.content = s.encode('UTF-8')

    def route(self):
        '''
Pierwszą rzeczą, którą aplikacja musi zrobić po odebraniu zapytania, jest
sprawdzenie nazwy metody HTTP oraz nazwy zasobu. Jest to konieczne aby się
zorientować o co klient prosi i wywołać odpowiedni fragment kodu realizujący
jego zlecenie. Jest to tzw. routing zapytania.

W niniejszej aplikacji routing jest realizowany częściowo w tej metodzie,
a częściowo w metodach handle_table() i handle_item().
'''
        path_info = self.env['PATH_INFO']
        if path_info == '/osoby':
            self.handle_table_osoby()
            return
        m = re.search('^/osoby/(?P<id>[0-9]+)$', self.env['PATH_INFO'])
        if m is not None:
            self.handle_item_osoby(m.group('id'))
            return
        if path_info == '/osoby/search':
            self.handle_search_osoby()
            return

        if path_info == '/psy':
            self.handle_table_psy()
            return
        m = re.search('^/psy/(?P<id>[0-9]+)$', self.env['PATH_INFO'])
        if m is not None:
            self.handle_psy(m.group('id'))
            return
        if path_info == '/psy/search':
            self.handle_search_psy()
            return
        self.failure('404 Not Found')

    def handle_table_osoby(self):
        '''
Obsługa zapytań odnoszących się do tabeli "osoby" traktowanej jako całość.
Można ją pobrać, albo można dodać do niej nowy wiersz.
'''
        if self.env['REQUEST_METHOD'] == 'GET':
            colnames, rows = self.sql_select()
            self.send_rows(colnames, rows)
        elif self.env['REQUEST_METHOD'] == 'POST':
            colnames, vals = self.read_tsv()
            q = 'INSERT INTO osoby (' + ', '.join(colnames) + ') VALUES ('
            q += ', '.join(['?' for v in vals]) + ')'
            id = self.sql_modify(q, vals)
            colnames, rows = self.sql_select(id)
            self.send_rows(colnames, rows)
        else:
            self.failure('501 Not Implemented')

    def handle_item_osoby(self, id):
        if self.env['REQUEST_METHOD'] == 'GET':
            colnames, rows = self.sql_select(id)
            if len(rows) == 0:
                self.failure('404 Not Found')
            else:
                self.send_rows(colnames, rows)
        elif self.env['REQUEST_METHOD'] == 'PUT':
            colnames, vals = self.read_tsv()
            q = 'UPDATE osoby SET '
            q += ', '.join([c + ' = ?' for c in colnames])
            q += ' WHERE id = ' + str(id)
            self.sql_modify(q, vals)
            colnames, rows = self.sql_select(id)
            self.send_rows(colnames, rows)
        elif self.env['REQUEST_METHOD'] == 'DELETE':
            q = 'DELETE FROM osoby WHERE id = ' + str(id)
            self.sql_modify(q)
        else:
            self.failure('501 Not Implemented')

    def handle_search_osoby(self):
        query_params = parse_qs(self.env['QUERY_STRING'])
        imie = query_params.get('imie', [None])[0]
        nazwisko = query_params.get('nazwisko', [None])[0]

        if self.env['REQUEST_METHOD'] == 'GET':
            if imie or nazwisko:
                colnames, rows = self.sql_select(None, imie, nazwisko)
                self.send_rows(colnames, rows)
            else:
                self.failure('400 Bad Request', 'Missing search parameters: imie or nazwisko')
        elif self.env['REQUEST_METHOD'] == 'PUT':
            colnames, vals = self.read_tsv()
            q = 'UPDATE osoby SET '
            q += ', '.join([c + ' = ?' for c in colnames])
            if imie is not None and nazwisko is not None:
                q += ' WHERE imie = ' + str(imie) + ' AND nazwisko = ' + str(nazwisko)
            elif imie is not None:
                q += ' WHERE imie = ' + str(imie)
            elif nazwisko is not None:
                q += ' WHERE nazwisko = ' + str(nazwisko)
            self.sql_modify(q, vals)
            colnames, rows = self.sql_select(None, imie, nazwisko)
            self.send_rows(colnames, rows)
        elif self.env['REQUEST_METHOD'] == 'DELETE':
            q = ""
            if imie is not None and nazwisko is not None:
                q = f"DELETE FROM osoby WHERE imie = '{imie}' AND nazwisko = '{nazwisko}'"
            elif imie is not None:
                q = f"DELETE FROM osoby WHERE imie = '{imie}'"
            elif nazwisko is not None:
                q = f"DELETE FROM osoby WHERE nazwisko = '{nazwisko}'"
            self.sql_modify(q)
        else:
            self.failure('501 Not Implemented')

    def handle_table_psy(self):
        if self.env['REQUEST_METHOD'] == 'GET':
            colnames, rows = self.sql_select_psy()
            self.send_rows(colnames, rows)
        elif self.env['REQUEST_METHOD'] == 'POST':
            colnames, vals = self.read_tsv()
            q = 'INSERT INTO psy (' + ', '.join(colnames) + ') VALUES ('
            q += ', '.join(['?' for v in vals]) + ')'
            id = self.sql_modify(q, vals)
            colnames, rows = self.sql_select_psy(id)
            self.send_rows(colnames, rows)
        else:
            self.failure('501 Not Implemented')

    def handle_psy(self, id):
        if self.env['REQUEST_METHOD'] == 'GET':
            colnames, rows = self.sql_select_psy(id)
            if len(rows) == 0:
                self.failure('404 Not Found')
            else:
                self.send_rows(colnames, rows)
        elif self.env['REQUEST_METHOD'] == 'PUT':
            colnames, vals = self.read_tsv()
            q = 'UPDATE psy SET '
            q += ', '.join([c + ' = ?' for c in colnames])
            q += ' WHERE id = ' + str(id)
            self.sql_modify(q, vals)
            colnames, rows = self.sql_select_psy(id)
            self.send_rows(colnames, rows)
        elif self.env['REQUEST_METHOD'] == 'DELETE':
            q = 'DELETE FROM psy WHERE id = ' + str(id)
            self.sql_modify(q)
        else:
            self.failure('501 Not Implemented')

    def handle_search_psy(self):
        query_params = parse_qs(self.env['QUERY_STRING'])
        imie = query_params.get('imie', [None])[0]
        rasa = query_params.get('rasa', [None])[0]

        if self.env['REQUEST_METHOD'] == 'GET':
            if imie or rasa:
                colnames, rows = self.sql_select_psy(None, imie, rasa)
                self.send_rows(colnames, rows)
            else:
                self.failure('400 Bad Request', 'Missing search parameters: imie or rasa')
        elif self.env['REQUEST_METHOD'] == 'PUT':
            colnames, vals = self.read_tsv()
            q = 'UPDATE psy SET '
            q += ', '.join([c + ' = ?' for c in colnames])
            if imie is not None and rasa is not None:
                q += ' WHERE imie = ' + str(imie) + ' AND rasa = ' + str(rasa)
            elif imie is not None:
                q += ' WHERE imie = ' + str(imie)
            elif rasa is not None:
                q += ' WHERE rasa = ' + str(rasa)
            self.sql_modify(q, vals)
            colnames, rows = self.sql_select_psy(None, imie, rasa)
            self.send_rows(colnames, rows)
        elif self.env['REQUEST_METHOD'] == 'DELETE':
            q = ""
            if imie is not None and rasa is not None:
                q = f"DELETE FROM psy WHERE imie = '{imie}' AND rasa = '{rasa}'"
            elif imie is not None:
                q = f"DELETE FROM psy WHERE imie = '{imie}'"
            elif rasa is not None:
                q = f"DELETE FROM psy WHERE rasa = '{rasa}'"
            self.sql_modify(q)
        else:
            self.failure('501 Not Implemented')

    def read_tsv(self):
        f = self.env['wsgi.input']
        n = int(self.env['CONTENT_LENGTH'])
        raw_bytes = f.read(n)
        lines = raw_bytes.decode('UTF-8').splitlines()
        colnames = lines[0].split('\t')
        vals = lines[1].split('\t')
        return colnames, vals

    def send_rows(self, colnames, rows):
        s = '\t'.join(colnames) + '\n'
        for row in rows:
            s += '\t'.join([str(val) for val in row]) + '\n'
        self.content = s.encode('UTF-8')
        self.headers = [('Content-Type',
                         'text/tab-separated-values; charset=UTF-8')]

    def sql_select(self, id=None, imie=None, nazwisko=None):
        conn = sqlite3.connect(plik_bazy)
        crsr = conn.cursor()
        query = 'SELECT * FROM osoby'
        if id is not None:
            query += f" WHERE id = '{id}'"
        elif imie is not None and nazwisko is not None:
            query += f" WHERE imie = '{imie}' AND nazwisko = '{nazwisko}'"
        elif imie is not None and nazwisko is None:
            query += f" WHERE imie = '{imie}'"
        elif imie is None and nazwisko is not None:
            query += f" WHERE nazwisko = '{nazwisko}'"
        print(query)
        crsr.execute(query)
        colnames = [d[0] for d in crsr.description]
        rows = crsr.fetchall()
        crsr.close()
        conn.close()
        return colnames, rows

    def sql_modify(self, query, params=None):
        conn = sqlite3.connect(plik_bazy)
        crsr = conn.cursor()
        if params is None:
            crsr.execute(query)
        else:
            crsr.execute(query, params)
        rowid = crsr.lastrowid  # id wiersza wstawionego przez INSERT
        crsr.close()
        conn.commit()
        conn.close()
        return rowid

    def sql_select_psy(self, id=None, imie=None, rasa=None):
        conn = sqlite3.connect(plik_bazy)
        crsr = conn.cursor()
        query = 'SELECT * FROM psy'
        if id is not None:
            query += f" WHERE id = '{id}'"
        elif imie is not None and rasa is not None:
            query += f" WHERE imie = '{imie}' AND rasa = '{rasa}'"
        elif imie is not None and rasa is None:
            query += f" WHERE imie = '{imie}'"
        elif imie is None and rasa is not None:
            query += f" WHERE rasa = '{rasa}'"
        print(query)
        crsr.execute(query)
        colnames = [d[0] for d in crsr.description]
        rows = crsr.fetchall()
        crsr.close()
        conn.close()
        return colnames, rows


if __name__ == '__main__':
    from wsgiref.simple_server import make_server

    port = 8000
    httpd = make_server('', port, OsobyApp)
    print('Listening on port %i, press ^C to stop.' % port)
    httpd.serve_forever()
