import re


def getLib(path):
    m = re.search('/\w*__.*?/', path)
    return m.group(0).strip('/')


def getName(lib):
    return lib.split('__')[0]


def getVersion(lib):
    return lib.split('__')[1]
