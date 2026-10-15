from pathlib import Path

def get_asset(file_name: str) -> str:
    assets_folder = Path(__file__).parent.resolve()
    return str(assets_folder / file_name)