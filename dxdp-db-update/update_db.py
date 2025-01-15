#!/usr/bin/env python3
sexit = False

try:
    import pandas as pd
except ImportError:
    print("Failed to import pandas...")
    print("Run source ./venv.sh then try again")
    sexit = True

if(sexit):
    sys.exit()

import requests
import os
from pathlib import Path
from datetime import datetime
import sys
from typing import List
import zipfile

ipv4_url = "https://download.ip2location.com/lite/IP2LOCATION-LITE-DB1.CSV.ZIP"
ipv6_url = "https://download.ip2location.com/lite/IP2LOCATION-LITE-DB1.IPV6.CSV.ZIP"

c_path = "/lib/dxdp/.dlcache"
path = Path(c_path)

ipv4_db_path_zip = "/lib/dxdp/tmp/ipv4.zip"
ipv6_db_path_zip = "/lib/dxdp/tmp/ipv6.zip"

ipv4_db_path_csv = "/lib/dxdp/ipv4.csv"
ipv6_db_path_csv = "/lib/dxdp/ipv6.csv"

def dl(url: str, path:str) -> datetime:
    response = requests.get(url, stream=True)
    response.raise_for_status()
    with open(path, 'wb') as file:
        for chunk in response.iter_content(chunk_size=8192):
            file.write(chunk)
    return datetime.strptime(response.headers.get('Last-Modified'), '%a, %d %b %Y %H:%M:%S %Z')

def db_fix_iploc(path:str, out:str) -> None:
    cn=["low","high","cc","cc2"]
    df = pd.read_csv(path, names=cn)
    df = df.drop('cc2', axis=1)
    df = df[df['cc'] != '-']
    columns = ['cc'] + [col for col in df.columns if col != 'cc']
    df = df[columns]
    df = df.replace('', None)
    df = df.dropna()
    df.to_csv(out, index=False, header=False)
    return

def write_f_line(path:str, line:int, val:str) -> None:
    with open(path, 'r') as file:
        lines = file.readlines()
    while len(lines) < line:
        lines.append('\n')
    lines[line - 1] = val + '\n'
    with open(path, 'w') as file:
        file.writelines(lines)

def unzip_csv_only(path:str, dest:str) -> None:
    with zipfile.ZipFile(path, 'r') as zip_ref:
        csv_files = [file for file in zip_ref.namelist() if file.lower().endswith('.csv')]
        if not csv_files:
            print("No .csv file found in the ZIP archive.")
            sys.exit()
            return
        if(len(csv_files) != 1):
            print("Too many CSVs")
            sys.exit()
            return
        csv_file = csv_files[0]

        with zip_ref.open(csv_file) as source, open(dest, 'wb') as target:
            target.write(source.read())

def del_f(path:str):
    os.remove(path)

if not path.exists():
    path.parent.mkdir(parents=True, exist_ok=True)
    path.touch()
    print("Creating cache file parent directories and/or file")

print("Reading cache file")
with open(c_path, "r") as file:
    lines = [line.strip() for line in file]

compare = False
corrupted = False
should_dl_ipv4 = False
should_dl_ipv6 = False
file_empty = False

if len(lines) == 2:
    compare = True
elif len(lines) != 0:
    corrupted = True
else:
    file_empty = True

if(compare):
    ipv4_lm_cache_str = lines[0]
    ipv4_lm_cache_date = datetime.fromisoformat(ipv4_lm_cache_str)
    ipv4_lm_head = requests.head(ipv4_url)
    ipv4_lm_str = ipv4_lm_head.headers['Last-Modified']
    ipv4_lm_date = datetime.strptime(ipv4_lm_str, '%a, %d %b %Y %H:%M:%S %Z')
    if(ipv4_lm_cache_date < ipv4_lm_date):
        should_dl_ipv4 = True
        print("IPV4 DB is outdated")
    else:
        print("IPV4 DB is current - no download needed")

    ipv6_lm_cache = lines[1]
    ipv4_lm_cache_date = datetime.fromisoformat(ipv6_lm_cache)
    ipv6_lm_head = requests.head(ipv6_url)
    ipv6_lm_str = ipv6_lm_head.headers['Last-Modified']
    ipv6_lm_date = datetime.strptime(ipv6_lm_str, '%a, %d %b %Y %H:%M:%S %Z')
    if(ipv4_lm_cache_date < ipv6_lm_date):
        should_dl_ipv6 = True
        print("IPV6 DB is outdated")
    else:
        print("IPV6 DB is current - no download needed")

if(file_empty):
    print("Cache download file is corrupted. Deleting and downloading db.")
    should_dl_ipv4 = True
    should_dl_ipv6 = True

if(file_empty):
    print("No cache...downloading dbs")
    should_dl_ipv4 = True
    should_dl_ipv6 = True
    
if(should_dl_ipv4):
    print("Downloading IPV4")
    lm = dl(ipv4_url, ipv4_db_path_zip)
    unzip_csv_only(ipv4_db_path_zip, ipv4_db_path_csv)
    db_fix_iploc(ipv4_db_path_csv, ipv4_db_path_csv)
    del_f(ipv4_db_path_zip)
    write_f_line(c_path, 1, lm.isoformat())
    print("Success")

if(should_dl_ipv6):
    print("Downloading IPV6 DB")
    lm = dl(ipv6_url, ipv6_db_path_zip)
    unzip_csv_only(ipv6_db_path_zip, ipv6_db_path_csv)
    db_fix_iploc(ipv6_db_path_csv, ipv6_db_path_csv)
    del_f(ipv6_db_path_zip)
    write_f_line(c_path, 2, lm.isoformat())
    print("Success")
