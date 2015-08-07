"""
Installation of Flask:
pip install Flask
"""

from flask import Flask, request

app = Flask(__name__, static_folder="explorer", static_url_path="")

@app.route('/save', methods=['POST'])
def save():
  if request.form['filename'] != 'sample.xml':
    raise ValueError("can only save 'sample.xml'")
  
  f = request.files['file_to_be_saved']

  f.save('explorer/sample.xml')
  return "saved ok"


if __name__ == "__main__":
    app.debug = True
    app.run()
