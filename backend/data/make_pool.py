#!/usr/bin/python
# coding: utf-8


import sys, base64, json, random, math


def ParseLine(item):
    try:
        return json.loads(base64.b64decode(item))
    except:
        return None


def ParseFile(path):
    data = []
    for line in open(path, "rt"):
        if line and line[-1] == "\n":
            line = line[:-1]
        item = ParseLine(line)
        if item:
            data.append(item)
    return data


def IterateSessions(data):
    data = sorted(data, key = lambda x: x["session_id"])
    prev_sess_id, session = None, []
    for item in data + [{"session_id": None}]:
        if prev_sess_id != item["session_id"]:
            if len(session) != 0:
                session = sorted(session, key = lambda x: x["client_timestamp"])
                yield session
            prev_sess_id, session = item["session_id"], []
        session.append(item)


def Color2Number(color):
    m = {'w': 0, 'r': 1, 'g': 2, 'y': 3, 'o': 4, 'b': 5}
    if color in m:
        return m[color]
    return None


def ExtractSubimage(data, i0, i1, j0, j1):
    result = []
    for i in xrange(i0, i1):
        row = []
        for j in xrange(j0, j1):
            row.append(data[i][j])
        result.append(row)
    return result


def ExtractFields(session):
    if len(session) == 0 or session[-1]["type"] != "answer" or session[-1]["state"] != "ok":
        return None, None
    pixels, colors = [None for i in xrange(54)], [None for i in xrange(54)]
    for item in session:
        if item["type"] == "fix_recognized":
            data = item["data"]
            surface = item["selected_surface"]
            for i in xrange(3):
                for j in xrange(3):
                    pixels[surface * 9 + i * 3 + j] = ExtractSubimage(data, i * 80, (i + 1) * 80, j * 80, (j + 1) * 80)
        elif item["type"] == "solve":
            cube = item["cube"]
            for i in xrange(6):
                colors[i * 9 + 4] = i
                for j in xrange(8):
                    color = Color2Number(cube[i * 8 + j])
                    k = j + (1 if j >= 4 else 0)
                    colors[i * 9 + k] = color
    for i in xrange(54):
        if pixels[i] == None or colors[i] == None:
            return None, None
    return pixels, colors


def MeanAndVariance(data):
    s, s2, n = 0.0, 0.0, 0
    for x in data:
        s += x
        s2 += x * x
        n += 1
    return [s / n, math.sqrt(s2 / n - s / n * s / n + 1e-10)]


def ExtractFeaturesFromPixelsArray(pixels, func):
    data = sorted(map(func, pixels))
    result = [data[100], data[200], data[300], data[400]]
    result += MeanAndVariance(data)
    return result
    

def ExtractFieldFeatures(pixels, idx):
    target_pixels, all_pixels = [], []
    for i in xrange(500):
        target_pixels.append(pixels[idx][int(random.random() * 80)][int(random.random() * 80)][:3])
        all_pixels.append(pixels[int(random.random() * 54)][int(random.random() * 80)][int(random.random() * 80)][:3])
    funcs = [lambda x: x[0], lambda x: x[1], lambda x: x[2]]
    result = []
    for func in funcs:
        result += ExtractFeaturesFromPixelsArray(target_pixels, func)
    return result


def OneHot(index, total):
    return [(1.0 if i == index else 0.0) for i in xrange(total)]


def ProcessFile(path, train_fout, test_fout):
    data = ParseFile(path)
    group_index = 0
    for session in IterateSessions(data):
        fout = train_fout if random.random() < 0.9 else test_fout
        types = map(lambda x: x["type"], session)
        #if len(filter(lambda x: x == "fix_recognized", types)) != 6:
        #    continue
        #print session[0]["session_id"], len(session), ",".join(types), (session[-1]["state"] if (session[-1]["type"] == "answer") else "") #, "\n\n"
        pixels, colors = ExtractFields(session)
        if pixels == None or colors == None:
            continue
        for field in xrange(54):
            for i in xrange(100):
                correct_color = colors[field]
                incorrect_color = int(random.random() * 5)
                if incorrect_color >= correct_color:
                    incorrect_color += 1
                features = ExtractFieldFeatures(pixels, field)
                line = [group_index, 1] + OneHot(correct_color, 6) + features
                fout.write("\t".join(map(str, line)) + "\n")
                line = [group_index, 0] + OneHot(incorrect_color, 6) + features
                fout.write("\t".join(map(str, line)) + "\n")
                group_index += 1


def main(train_path, test_path, files):
    train_fout = open(train_path, "wt")
    test_fout = open(test_path, "wt")
    for path in files:
        ProcessFile(path, train_fout, test_fout)


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2], sys.argv[3:])

